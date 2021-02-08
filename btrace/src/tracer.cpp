#include "tracer.h"

#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <list>
#include <map>
#include <set>
#include <sstream>

#include "rapidjson/writer.h"
#include "rapidjson/filewritestream.h"

namespace {

bool myReadlinkAt(int fd, const char* path, std::string& res) {
    std::vector<char> buf(1024);
    while (1) {
        ssize_t r = readlinkat(fd, path, &buf[0], buf.size());
        if (r == -1) {
            return false;
        }
        if (r >= buf.size()) {
            buf.resize(2 * buf.size());
        } else {
            res = std::string(&buf[0], &buf[r]);
            return true;
        }
    }
}

/**
 * Read value and assign string.
 *
 * Return false if failed and set `errorAddr` and `errno`.
 */
bool readChars(pid_t p, uint64_t addr, std::string& s, uint64_t& errorAddr) {
    uint64_t base = addr & 0xfffffffffffffff8;
    uint64_t off = addr - base;
    std::vector<char> bytes;
    // Read
    uint64_t r = ptrace(PTRACE_PEEKDATA, p, base, 0);
    if ((r == (uint64_t)-1) && errno) {
        errorAddr = base;
        return false;
    }
    r = r >> (8*off);
    for (size_t i=0; i != 8-off; ++i) {
        char c = r;
        bytes.push_back((char) r);
        if (!c) {
            s = &bytes[0];
            return true;
        }
        r = r >> 8;
    }
    while (1) {
        base += 8;
        uint64_t r = ptrace(PTRACE_PEEKDATA, p, base, 0);
        if ((r == (uint64_t)-1) && errno) {
            errorAddr = base;
            return false;
        }
        for (size_t i=0; i != 8; ++i) {
            char c = r;
            bytes.push_back((char) r);
            if (!c) {
                s = &bytes[0];
                return true;
            }
            r = r >> 8;
        }
    }
}

bool readArgs(pid_t p, uint64_t addr, std::vector<std::string>& res, uint64_t& errorAddr) {
    while (1) {
        uint64_t r = ptrace(PTRACE_PEEKDATA, p, addr, 0);
        if ((r == (uint64_t)-1) && errno) {
            errorAddr = addr;
            return false;
        }
        if (!r) return true;
        std::string a;
        uint64_t errorAddr;
        if (!readChars(p, r, a, errorAddr)) {
            return false;
        }
        res.push_back(a);
        addr += 8;
    }
}

///////////////////////////////////////////////////////////////////////////////
// execve_t

/**
 * Record of information from an execve call.
 */
class execve_t {
public:
    /**
     * Command name passed to execve
     *
     * Note.  This may be a path relative to the cwd and either
     * an executable or a script.  In scripts, the exePath will
     * be set to the interpreter rather than cmd.
     */
    std::string cmd;
    std::vector<std::string> args;
    std::vector<std::string> env;
    /** Indicates if we saw the return for the execve. */
    bool returned = false;
    /** Only defined if returned is true. */
    uint64_t returnVal;
    /**
     * Path for actual executable (empty if could not be retrieved).
      */
    std::string exePath;
    /** Path for current working directory (empty if could not be retrieved). */
    std::string cwdPath;
};

///////////////////////////////////////////////////////////////////////////////
// process_t

/**
 * Information about a process.
 */
class process_t {
public:
    process_t() = default;
    process_t(process_t&&) = default;
    process_t(const process_t&) = delete;
    process_t& operator=(const process_t&) = delete;

    /** Flag to indicate f process is running. */
    bool running = false;
    bool newClone = false;
    bool inSyscall = false;
    /**
     *  Last system call value.  Only valid if inSyscall is true.
     */
    uint64_t lastSyscall;

    /**
     * Execve's called by process.
     */
    std::vector<execve_t> execve;
    std::vector<pid_t> children;
};

class state_t;

void printProcess(const state_t& s, int indent, pid_t p);

void loop(state_t& s);

/** State for tracking ptrace event. */
class state_t {
    // Flag to indicate if we should output debug information.
    const bool debug;
    // File pointer to log events to.
    FILE* const f;
    // Identifieres of processes that are still alive.
    std::set<pid_t> alive;
public:
    state_t(bool debug, FILE* f) : debug(debug), f(f) {
    }

    state_t(const state_t&) = delete;
    state_t& operator=(const state_t&) = delete;

    /**
     * Return true if there are processes that are alive.
     */
    bool hasAliveProcesses() const {
        return this->alive.size() > 0;
    }

    std::map<pid_t, process_t> processMap;

    std::set<uint64_t> unknownSyscalls;
    std::list<std::pair<pid_t, std::string>> messages;

    process_t& startMonitoring(pid_t p) {
        auto r = this->processMap.emplace(p, process_t());
        // If element inserted
        if (r.second) {
            if (!this->alive.insert(p).second) {
                errorLog(p, "process already inserted in alive set.");
            }
            r.first->second.running = true;
        } else {
            errorLog(p, "process already inserted in process map");
        }
        return r.first->second;
    }

    void stopMonitoring(pid_t p, process_t& ps) {
        size_t cnt = this->alive.erase(p);
        if (cnt == 0 || !ps.running) {
            errorLog(p, "internal error: stopMonitoring when already stopped.");
        }
        ps.running = false;
    }

    /**
     * Mark that we have entered a system call and we expect
     * next event to provide result.
     */
    void syscallEnterDone(pid_t p, process_t& ps, uint64_t syscallNo);

    /**
     * Mark that we have entered a system call and we expect
     * next event to provide result.
     */
    void syscallExitDone(pid_t p, process_t& ps, uint64_t syscallNo);

    /**
     * Resume process until next PTRACE system call or other event.
     *
     * If @signal@ is provided and non-zero then the process is sent that signal.
     */
    void ptraceSyscall(pid_t p, process_t& ps, int signal = 0) {
        if (ptrace(PTRACE_SYSCALL, p, 0, signal)) {
            errorLog(p, "ptrace(PTRACE_SYSCALL, ...) failed (errno = %d).\n", p, errno);
            stopMonitoring(p, ps);
        }
    }

    void resumeWithoutMonitoring(pid_t p) {
        if (ptrace(PTRACE_CONT, p, 0, 0)) {
            errorLog(p, "ptrace(PTRACE_CONT, ...) failed (errno = %d).\n", p, errno);
        }
    }

    void debugLog(pid_t p, const char* fmt, ...) const;
    void errorLog(pid_t p, const char* fmt, ...);

    friend void loop(state_t& s);
};

void debugDump(pid_t p, const std::set<pid_t>& alive, const char* msg) {
    std::stringstream s;
    s << "Process " << p << " (";
    bool seen = false;
    for (auto a : alive) {
        if (seen) { s << ", "; }
        s << a;
        seen = true;
    }
    s << "): " << msg << std::endl;
    std::string str = s.str();
    write(STDERR_FILENO, str.c_str(), str.length());

}
void state_t::debugLog(pid_t p, const char* fmt, ...) const {
    if (!this->debug) return;
    char* msg;
    va_list vl;
    va_start(vl, fmt);
    int cnt = vasprintf(&msg, fmt, vl);
    va_end(vl);
    if (cnt == -1) {
        const char* msg = "internal error: log failed.\n";
        write(STDERR_FILENO, msg, strlen(msg));
        return;
    }
    debugDump(p, this->alive, msg);
    free(msg);
}

void state_t::errorLog(pid_t p, const char* fmt, ...) {
    char* s;
    va_list vl;
    va_start(vl, fmt);
    int cnt = vasprintf(&s, fmt, vl);
    va_end(vl);
    if (cnt == -1) {
        const char* msg = "internal error: log failed.\n";
        this->messages.push_back(std::make_pair(p, std::string(msg)));
        write(STDERR_FILENO, msg, strlen(msg));
        return;
    }

    messages.push_back(std::make_pair(p, s));
    fprintf(this->f, "Process %d: %s", p, s);
    if (debug) {
        debugDump(p, this->alive, s);
    }
    free(s);
}

void printProcess(const state_t& s, FILE* f, int indent, pid_t p) {
    fprintf(f, "%*sprocess %d\n", indent, "", p);
    auto iProc = s.processMap.find(p);
    if (iProc == s.processMap.end()) {
        fprintf(f, "%*sinfo missing\n", indent+2, "");
    } else {
        const process_t& ps = iProc->second;

        for (const auto& e : ps.execve) {
            fprintf(f, "%*sexecve\n", indent + 2, "");
            fprintf(f, "%*scmd = %s\n", indent + 4, "", e.cmd.c_str());
            fprintf(f, "%*scwd = %s\n", indent + 4, "", e.cwdPath.c_str());
            fprintf(f, "%*sexe = %s\n", indent + 4, "", e.exePath.c_str());
            for (size_t i = 0; i != e.args.size(); ++i) {
                fprintf(f, "%*sarg[%zu] = %s\n", indent + 4, "", i, e.args[i].c_str());
            }
        }
        for (const auto& pc : ps.children) {
            printProcess(s, f, indent + 2, pc);
        }
    }
}

void print(const state_t& s, FILE* f, pid_t p) {
    printProcess(s, f, 0, p);
    for (auto c : s.unknownSyscalls) {
        fprintf(f, "Unknown system call: %ld.\n", c);
    }

    for (const auto& m : s.messages) {
        fprintf(f, "Error %d: %s", m.first, m.second.c_str());
    }
}

template<typename Stream>
void printProcessJSON(rapidjson::Writer<Stream>& writer, const state_t& s, pid_t p) {
    writer.StartObject();
    writer.Key("pid");
    writer.Int(p);

    auto iProc = s.processMap.find(p);
    if (iProc != s.processMap.end()) {
        const process_t& ps = iProc->second;
        if (!ps.execve.empty()) {
            writer.Key("execve");
            writer.StartArray();
            for (const auto& e : ps.execve) {
                writer.StartObject();
                writer.Key("cmd");
                writer.String(e.cmd.c_str());
                writer.Key("cwd");
                writer.String(e.cwdPath.c_str());
                writer.Key("exe");
                writer.String(e.exePath.c_str());
                writer.Key("args");
                writer.StartArray();
                for (const auto& a : e.args) {
                    writer.String(a.c_str());
                }
                writer.EndArray();
                writer.EndObject();
            }
            writer.EndArray();
        }
        if (!ps.children.empty()) {
            writer.Key("children");
            writer.StartArray();
            for (const auto& pc : ps.children) {
                printProcessJSON(writer, s, pc);
            }
            writer.EndArray();
        }
    }
    writer.EndObject();
}

void printJSON(const state_t& s, FILE* f, pid_t p) {
    char writeBuffer[4092];
    rapidjson::FileWriteStream os(f, writeBuffer, sizeof(writeBuffer));

    rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
    writer.StartObject();
    writer.Key("process");
    printProcessJSON(writer, s, p);

    if (!s.unknownSyscalls.empty()) {
        writer.Key("unknown_syscalls");
        writer.StartArray();
        for (auto c : s.unknownSyscalls) {
            writer.Uint64(c);
        }
        writer.EndArray();
    }
    if (!s.messages.empty()) {
        writer.Key("errors");
        writer.StartArray();
        for (const auto& m : s.messages) {
            writer.StartObject();
            writer.Key("pid");
            writer.Int(m.first);
            writer.Key("message");
            writer.String(m.second.c_str());
            writer.EndObject();
        }
        writer.EndArray();
    }
    writer.EndObject();
}

/**
 * Mark that we have entered a system call and we expect
 * next event to provide result.
 */
void state_t::syscallEnterDone(pid_t p, process_t& ps, uint64_t syscallNo) {
    ps.inSyscall = true;
    ps.lastSyscall = syscallNo;
    this->ptraceSyscall(p, ps);
}

void state_t::syscallExitDone(pid_t p, process_t& ps, uint64_t syscallNo) {
    if (!ps.inSyscall) {
        errorLog(p, "Entered syscall when already in one.\n");
    } else if (ps.lastSyscall != syscallNo) {
        errorLog(p, "Syscall enter %d not matched by syscall exit %d.\n", ps.lastSyscall, syscallNo);
    }
    ps.inSyscall = false;
    this->ptraceSyscall(p, ps);
}

/**
 * Populate cwdPath and exePath
 */
void populatwCwdExe(state_t& s, pid_t p, execve_t& execveState) {
    char* dirPath;
    if (asprintf(&dirPath, "/proc/%d", p) == -1) {
        s.errorLog(p, "Error in asprintf (errno = %d).\n", errno);
        return;
    }
    int dirFd = open(dirPath, O_DIRECTORY | O_PATH);
    if (dirFd == -1) {
        free(dirPath);
        s.errorLog(p, "Error opening procfs dir (errno = %d).\n", errno);
        return;
    }
    free(dirPath);

    if (!myReadlinkAt(dirFd, "cwd", execveState.cwdPath)) {
        s.errorLog(p, "Failed to read cwd path (errno = %d)\n", errno);
    }
    if (!myReadlinkAt(dirFd, "exe", execveState.exePath)) {
        s.errorLog(p, "Failed to read exe path (errno = %d)\n", errno);
    }
}

void execveInvoke(state_t& s, pid_t p, process_t& ps, const struct user_regs_struct& regs) {
    ps.execve.emplace_back();
    execve_t& e = ps.execve.back();
    uint64_t errorAddr;
    if (!readChars(p, regs.rdi, e.cmd, errorAddr)) {
        s.errorLog(p, "Error reading execve path (addr = %lu, errno = %d).\n", errorAddr, errno);
    } else if (!readArgs(p, regs.rsi, e.args, errorAddr)) {
        s.errorLog(p, "Error reading execve args (addr = %lu;, errno = %d).\n", errorAddr, errno);
    } else if (!readArgs(p, regs.rdx, e.env, errorAddr)) {
        s.errorLog(p, "Error reading execve env (addr = %lu, errno = %d).\n", errorAddr, errno);
    }
    s.syscallEnterDone(p, ps, SYS_execve);
}

/**
 * execve return when it fails.
 */
void execveReturn(state_t& s, pid_t p, process_t& ps, const struct user_regs_struct& regs) {
    if (ps.execve.empty()) {
        s.errorLog(p, "execve return unmatched.\n");
        return;
    }
    execve_t& e = ps.execve.back();
    e.returned = true;
    e.returnVal = regs.rax;
    if (regs.rax != 0) {
        s.errorLog(p, "execve failed (error = %llu)\n", regs.rax);
        return;
    }

    populatwCwdExe(s, p, e);
}

static void clonevforkReturn(state_t& s, pid_t p, process_t& ps, const struct user_regs_struct& regs) {
    pid_t newPid = regs.rax;
    ps.children.push_back(newPid);
    if (ptrace(PTRACE_ATTACH, newPid, NULL, NULL)) {
        s.errorLog(newPid, "ptrace(PTRACE_ATTACH, ...) failed (errno = %d).\n", errno);
        return;
    }
    process_t& newPS = s.startMonitoring(newPid);
    newPS.newClone = true;
}

std::set<uint64_t> ignoredSyscalls = {
    SYS_read, // 0
    SYS_write, // 1
    SYS_close, // 3
    SYS_stat, // 4
    SYS_fstat, // 5
    SYS_lstat, // 6
    SYS_lseek, // 8
    SYS_mmap, // 9
    SYS_mprotect, // 10
    SYS_munmap, // 11
    SYS_brk, // 12
    SYS_rt_sigaction, // 13
    SYS_rt_sigprocmask, // 14
    SYS_ioctl, // 16
    SYS_pread64, // 17
    SYS_access, // 21
    SYS_pipe, // 22
    SYS_select, // 23
    SYS_mremap, // 25
    SYS_dup2, // 33
    SYS_getpid, // 39
    SYS_socket, // 41
    SYS_connect, // 42
    SYS_wait4, // 61
    SYS_uname, // 63
    SYS_fcntl, // 72
    SYS_getcwd, // 79
    SYS_chdir, // 80
    SYS_rename, // 82
    SYS_mkdir, // 83
    SYS_rmdir, // 84
    SYS_unlink, // 87
    SYS_readlink, // 89
    SYS_chmod, // 90
    SYS_chown, // 92
    SYS_umask, // 95
    SYS_gettimeofday, // 96
    SYS_getrusage, // 98
    SYS_sysinfo, // 99
    SYS_ptrace, // 101
    SYS_getuid, // 102
    SYS_getgid, // 104
    SYS_geteuid, // 107
    SYS_getegid, // 108
    SYS_getppid, // 110
    SYS_getpgrp, // 111
    SYS_sigaltstack, // 131
    SYS_statfs, // 137
    SYS_arch_prctl, // 158
    SYS_gettid, // 186
    SYS_getxattr, // 191
    SYS_lgetxattr, // 192
    SYS_futex, // 202
    SYS_getdents64, // 217
    SYS_set_tid_address, // 218
    SYS_clock_gettime, // 228
    SYS_clock_getres, // 229
    SYS_exit_group, // 231
    SYS_tgkill, // 234
    SYS_openat, // 257
    SYS_set_robust_list, // 273
    SYS_eventfd2, // 290
    SYS_epoll_create1, // 291
    SYS_pipe2, // 293
    SYS_prlimit64, // 302
    SYS_getrandom, // 318
};

static void handleStop(state_t& s, pid_t p, process_t& ps, int status) {
    int signal = WSTOPSIG(status);
    switch (signal) {
    // If stopped due to system call.
    case 0x85:
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, p, NULL, &regs)) {
            s.errorLog(p, "ptrace(PTRACE_GETREGS, ..) failed (errno = %d).\n", errno);
            s.stopMonitoring(p, ps);
            s.resumeWithoutMonitoring(p);
            return;
        }
        if (!ps.inSyscall) {
            s.debugLog(p, "syscall enter %d", regs.orig_rax);
            switch (regs.orig_rax) {
            case SYS_rt_sigreturn: // 15
                // sigreturn does not return so we do not check for erro
                s.syscallEnterDone(p, ps, SYS_clone);
                break;
            case SYS_clone: // 56
                s.syscallEnterDone(p, ps, SYS_clone);
                break;
            case SYS_vfork: // 58
                s.syscallEnterDone(p, ps, SYS_vfork);
                break;
            case SYS_execve: // 59
                execveInvoke(s, p, ps, regs);
                break;
            default:
                if (ignoredSyscalls.count(regs.orig_rax) == 0)
                    s.unknownSyscalls.insert(regs.orig_rax);
                s.syscallEnterDone(p, ps, regs.orig_rax);
                break;
            }
        } else {
            s.debugLog(p, "syscall exit %d", regs.orig_rax);
            switch ((int64_t) regs.orig_rax) {
            case -1: // Special error code for no-return calls.
                if (   ps.lastSyscall != SYS_rt_sigreturn
                    && ps.lastSyscall != SYS_clone) {
                    s.errorLog(p, "syscall error on %ld\n", ps.lastSyscall);
                }
                break;
            case SYS_clone: // 56
            case SYS_vfork: // 58
                if (ps.lastSyscall != regs.orig_rax) {
                    s.errorLog(p, "syscall exit %ld (expected = %d)\n",
                       regs.orig_rax, ps.lastSyscall);
                }
                clonevforkReturn(s, p, ps, regs);
                break;
            case SYS_execve: // 59
                if (ps.lastSyscall != regs.orig_rax) {
                    s.errorLog(p, "syscall exit %ld (expected = %d)\n",
                       regs.orig_rax, ps.lastSyscall);
                }
                execveReturn(s, p, ps, regs);
                break;
            default:
                if (ps.lastSyscall != regs.orig_rax) {
                    s.errorLog(p, "syscall exit %ld (expected = %d)\n",
                       regs.orig_rax, ps.lastSyscall);
                }
                break;
            }
            ps.inSyscall = false;
            s.ptraceSyscall(p, ps);
        }
        break;
    case SIGTRAP:
        s.debugLog(p, "SIGTRAP");
        // Supress exec triggered traps.
        {
            bool isExecTrap = status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8));
            if (!isExecTrap) {
//                s.errorLog(p, "Unexpected SIGTRAP %x.\n", status >> 8);
                s.ptraceSyscall(p, ps, 0);
            } else if (!(ps.inSyscall && ps.lastSyscall == SYS_execve)) {
                s.errorLog(p, "Exec SIGTRAP not in execve.\n", status >> 8);
                s.ptraceSyscall(p, ps, 0);
            } else {
                s.ptraceSyscall(p, ps, 0);
            }
        }
        break;
    case SIGCHLD:
        s.debugLog(p, "SIGCHLD");
        s.ptraceSyscall(p, ps, SIGCHLD);
        break;
    case SIGSTOP:
        if (ps.newClone) {
            s.debugLog(p, "Expected SIGSTOP in new clone.");
            ps.newClone = false;
            if (ptrace(PTRACE_SETOPTIONS, p, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC)) {
                switch (errno) {
                case ESRCH:
                    s.errorLog(p, "tracesysgood failed - no such process.\n", p);
                default:
                    s.errorLog(p, "tracesysgood failed %d.\n", p, errno);
                }
                s.stopMonitoring(p, ps);
                s.resumeWithoutMonitoring(p);
            } else {
                s.ptraceSyscall(p, ps, SIGSTOP);
            }
        } else {
            s.ptraceSyscall(p, ps, SIGSTOP);
        }
        break;
    default:
        s.debugLog(p, "SIGNAL %d", WSTOPSIG(status));
        s.errorLog(p, "Unexpected signal %d.", WSTOPSIG(status));
        s.ptraceSyscall(p, ps, signal);
    }
}

void loop(state_t& s) {
    int status;
    pid_t p = wait(&status);
    if (p == -1) {
        switch (errno) {
        case ECHILD:
            s.errorLog(p, "wait failed due to no children.\n");
            return;
        case EINTR:
            s.errorLog(p, "Unexpected signal in wait.\n");
            return;
        default:
            s.errorLog(p, "wait failed %d.\n", errno);
            return;
        }
    }

    auto ip = s.processMap.find(p);
    if (ip == s.processMap.end()) {
        s.errorLog(p, "Unexpected tracee.\n");
        s.resumeWithoutMonitoring(p);
        return;
    }
    process_t& ps = ip->second;
    if (!ps.running) {
        s.errorLog(p, "Unexpected tracee event after terminal failure.\n");
        s.resumeWithoutMonitoring(p);
        return;
    }
    if (WIFEXITED(status)) {
        s.debugLog(p, "Exit");
        s.stopMonitoring(p, ps);
    } else if (WIFSIGNALED(status)) {
        s.debugLog(p, "Terminal signal");
        s.stopMonitoring(p, ps);
    } else if (WIFSTOPPED(status)) {
        handleStop(s, p, ps, status);
    } else if (WIFCONTINUED(status)) {
        s.debugLog(p, "Continued");
        s.errorLog(p, "Unexpected continue.\n");
        s.ptraceSyscall(p, ps);
    } else {
        s.debugLog(p, "Unexpected wait %d", status);
        s.errorLog(p, "Unexpected wait.\n");
        s.ptraceSyscall(p, ps);
    }
}

void run(state_t& s, const Params& params, pid_t p) {
    int status;
    int r = waitpid(p, &status, 0);
    if (r == -1) {
        s.errorLog(p, "waitpid failed %d.\n", errno);
        kill(p, SIGKILL);
        return;
    }

    // If execve in child thread succeeds, then next event will be a trap.
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        // Do nothing
    // Otherwise it should be a exit as execve failed.
    } else if (WIFEXITED(status)) {
        s.errorLog(p, "Could not run %s (errno = %d).\n", params.cmd.c_str(), WEXITSTATUS(status));
        // Quit (child printed error message).
        return;
    } else {
        s.errorLog(p, "Unexpected result from fork (status = %d).\n", status);
        kill(p, SIGKILL);
        return;
    }

    if (ptrace(PTRACE_SETOPTIONS, p, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC)) {
        s.errorLog(p, "ptrace(PTRACE_SETOPTIONS, ..) failed (errno = %d).\n", errno);
        kill(p, SIGKILL);
        return;
    }

    process_t& ps = s.startMonitoring(p);

    // Initialize first command
    ps.execve.emplace_back();
    execve_t& initCmd = ps.execve.back();
    initCmd.cmd = params.cmd;
    initCmd.args = params.args;
    for (char*const* e=params.envp; *e; ++e) {
        initCmd.env.push_back(*e);
    }
    initCmd.env.shrink_to_fit();
    initCmd.returned = true;
    initCmd.returnVal = 0;
    populatwCwdExe(s, p, initCmd);
    // Resume from trap now that state is setup.
    s.ptraceSyscall(p, ps);
    // Run
    while (s.hasAliveProcesses()) {
        loop(s);
    }
}

}

int btrace(const Params& params) {
    // Launch application
    pid_t p = fork();
    // Child process
    if (p == 0) {
        ptrace(PTRACE_TRACEME);
        if (params.stdout != -1) {
            dup2(params.stdout, STDOUT_FILENO);
        }
        if (params.stdout != -1) {
            dup2(params.stderr, STDERR_FILENO);
        }
        std::vector<char*> execArgs;
        execArgs.reserve(params.args.size() + 1);
        for (auto& a : params.args)
            execArgs.push_back((char*) a.c_str());
        execArgs.push_back(0);
        int r = execve(params.cmd.c_str(), &execArgs[0], params.envp);
        exit(errno);
    }

    state_t s(params.debug, params.output);
    run(s, params, p);
    if (params.jsonOutput) {
        printJSON(s, params.output, p);
    } else {
        print(s, params.output, p);
    }
    bool good = s.unknownSyscalls.empty() && s.messages.empty();
    return good ? 0 : -1;
}