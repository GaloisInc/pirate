#include "tracer.h"

#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/ptrace.h>
#include <sys/signalfd.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <list>
#include <map>
#include <set>
#include <sstream>

#include "event.h"

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
};

class state_t;

void printProcess(const state_t& s, int indent, pid_t p);

void loop(state_t& s);

/** State for tracking ptrace event. */
class state_t {
    // Flag to indicate if we should output debug information.
    const bool debug;
    // File pointer to log events to.
    // FILE* const f;
    // Identifieres of processes that are still alive.
    std::set<pid_t> alive;
    char writeBuffer[4092];
    // Has errors
    bool fHasErrors = false;
public:
    rapidjson::FileWriteStream os;
    // Set of executable paths that we do not trace.
    std::set<std::string> untracedExes;

    state_t(bool debug)
      : debug(debug),
        os(::stdout, writeBuffer, sizeof(writeBuffer)) {
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

    bool hasErrors() const { return fHasErrors; }

    process_t& startMonitoring(pid_t p) {
        auto r = this->processMap.emplace(p, process_t());
        // If element inserted
        if (r.second) {
            if (!this->alive.insert(p).second) {
                logError(os, p, "process already inserted in alive set.");
            }
            r.first->second.running = true;
        } else {
            logError(os, p, "process already inserted in process map.");
        }
        return r.first->second;
    }

    /**
     * Record that we will stop monitoring this process.
     */
    void stopMonitoring(pid_t p, process_t& ps) {
        size_t cnt = this->alive.erase(p);
        if (cnt == 0 || !ps.running) {
            logError(os, p, "internal error: stopMonitoring when already stopped.");
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
    void resumeUntilSyscall(pid_t p, process_t& ps, int signal = 0) {
        if (ptrace(PTRACE_SYSCALL, p, 0, signal)) {
            logFatalError(this->os, p, "ptrace(PTRACE_SYSCALL, ...) failed (errno = %d).", p, errno);
            stopMonitoring(p, ps);
        }
    }

    /**
     * Resume process p, but no longer monitor output.
     *
     * Note.  This will not change process liveness status, so the process should no
     * longer be alive (e.g., call stopMonitoring first).
     */
    void resumeWithoutMonitoring(pid_t p) {
        if (ptrace(PTRACE_CONT, p, 0, 0)) {
            logError(os, p, "ptrace(PTRACE_CONT, ...) failed (errno = %d).", errno);
        }
    }

    void debugLog(pid_t p, const char* fmt, ...) const;

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
        const char* msg = "internal error: log failed.";
        write(STDERR_FILENO, msg, strlen(msg));
        return;
    }
    debugDump(p, this->alive, msg);
    free(msg);
}

/**
 * Mark that we have entered a system call and we expect
 * next event to provide result.
 */
void state_t::syscallEnterDone(pid_t p, process_t& ps, uint64_t syscallNo) {
    ps.inSyscall = true;
    ps.lastSyscall = syscallNo;
    this->resumeUntilSyscall(p, ps);
}

void state_t::syscallExitDone(pid_t p, process_t& ps, uint64_t syscallNo) {
    if (!ps.inSyscall) {
        logError(os, p, "Entered syscall when already in one.");
    } else if (ps.lastSyscall != syscallNo) {
        logError(os, p, "Syscall enter %d not matched by syscall exit %d.", ps.lastSyscall, syscallNo);
    }
    ps.inSyscall = false;
    this->resumeUntilSyscall(p, ps);
}

/**
 * Populate cwdPath and exePath from proc address space.
 *
 * @return True if we should trace this process.
 */
bool populateCwdExe(state_t& s, pid_t p, execve_t& execveState) {
    char* dirPath;
    if (asprintf(&dirPath, "/proc/%d", p) == -1) {
        logError(s.os, p, "Error in asprintf (errno = %d).", errno);
        return false;
    }
    int dirFd = open(dirPath, O_DIRECTORY | O_PATH);
    if (dirFd == -1) {
        free(dirPath);
        logError(s.os, p, "Error opening procfs dir (errno = %d).", errno);
        return false;
    }
    free(dirPath);

    if (!myReadlinkAt(dirFd, "cwd", execveState.cwdPath)) {
        logError(s.os, p, "Failed to read cwd path (errno = %d).", errno);
    }
    if (!myReadlinkAt(dirFd, "exe", execveState.exePath)) {
        logError(s.os, p, "Failed to read exe path (errno = %d).", errno);
    }
    bool traced = s.untracedExes.count(execveState.cmd) == 0;
    printExecve(s.os, p, execveState, traced);
    return traced;
}

void execveInvoke(state_t& s, pid_t p, process_t& ps, const struct user_regs_struct& regs) {
    ps.execve.emplace_back();
    execve_t& e = ps.execve.back();
    uint64_t errorAddr;
    if (!readChars(p, regs.rdi, e.cmd, errorAddr)) {
        logError(s.os, p, "Error reading execve path (addr = %lu, errno = %d).", errorAddr, errno);
    } else if (!readArgs(p, regs.rsi, e.args, errorAddr)) {
        logError(s.os, p, "Error reading execve args (addr = %lu;, errno = %d).", errorAddr, errno);
    } else if (!readArgs(p, regs.rdx, e.env, errorAddr)) {
        logError(s.os, p, "Error reading execve env (addr = %lu, errno = %d).", errorAddr, errno);
    }
    s.syscallEnterDone(p, ps, SYS_execve);
}

/**
 * execve return when it fails.
 */
void execveReturn(state_t& s, pid_t p,  process_t& ps, const struct user_regs_struct& regs) {
    if (ps.execve.empty()) {
        logError(s.os, p, "execve return unmatched.");
        return;
    }
    execve_t& e = ps.execve.back();
    if (regs.rax != 0) {
        logError(s.os, p, "execve failed (error = %llu).", regs.rax);
        return;
    }
    if (populateCwdExe(s, p, e)) {
        s.resumeUntilSyscall(p, ps);
    } else {
        ps.inSyscall = false;
        s.stopMonitoring(p, ps);
        s.resumeWithoutMonitoring(p);
    }

}

static void clonevforkReturn(state_t& s, pid_t p, const struct user_regs_struct& regs) {
    pid_t newPid = regs.rax;
    printClone(s.os, p, newPid);
    if (ptrace(PTRACE_ATTACH, newPid, NULL, NULL)) {
        switch (errno) {
        case EPERM:
            logError(s.os, newPid, "Process already being traced.", errno);
            break;
        default:
            logError(s.os, newPid, "ptrace(PTRACE_ATTACH, ...) failed (errno = %d).", errno);
            break;

        }
        return;
    }
    process_t& newPS = s.startMonitoring(newPid);
    newPS.newClone = true;
}

/**
 * Called to respond to notice that process has stopped.
 *
 * @status Status value returned by wait.
 */
static void processStopped(state_t& s, pid_t p, process_t& ps, int status) {
    int signal = WSTOPSIG(status);
    switch (signal) {
    // If stopped due to system call.
    case 0x85:
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, p, NULL, &regs)) {
            logFatalError(s.os, p, "ptrace(PTRACE_GETREGS, ..) failed (errno = %d).", errno);
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
                s.syscallEnterDone(p, ps, regs.orig_rax);
                break;
            }
        } else {
            s.debugLog(p, "syscall exit %d", regs.orig_rax);
            ps.inSyscall = false;
            switch ((int64_t) regs.orig_rax) {
            case -1: // Special error code for no-return calls.
                if (   ps.lastSyscall != SYS_rt_sigreturn
                    && ps.lastSyscall != SYS_clone) {
                    logError(s.os, p, "syscall error on %ld.", ps.lastSyscall);
                }
                s.resumeUntilSyscall(p, ps);
                break;
            case SYS_clone: // 56
            case SYS_vfork: // 58
                if (ps.lastSyscall != regs.orig_rax) {
                    logError(s.os, p, "syscall exit %ld (expected = %d).",
                       regs.orig_rax, ps.lastSyscall);
                }
                clonevforkReturn(s, p, regs);
                s.resumeUntilSyscall(p, ps);
                break;
            case SYS_execve: // 59
                if (ps.lastSyscall != regs.orig_rax) {
                    logError(s.os, p, "syscall exit %ld (expected = %d).",
                       regs.orig_rax, ps.lastSyscall);
                }
                execveReturn(s, p, ps, regs);
                break;
            default:
                if (ps.lastSyscall != regs.orig_rax) {
                    logError(s.os, p, "syscall exit %ld (expected = %d).",
                       regs.orig_rax, ps.lastSyscall);
                }
                s.resumeUntilSyscall(p, ps);
                break;
            }
        }
        break;
    case SIGTRAP:
        s.debugLog(p, "SIGTRAP");
        // Supress exec triggered traps.
        {
            bool isExecTrap = status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC<<8));
            if (!isExecTrap) {
                s.resumeUntilSyscall(p, ps, 0);
            } else if (!(ps.inSyscall && ps.lastSyscall == SYS_execve)) {
                logError(s.os, p, "Exec SIGTRAP not in execve.", status >> 8);
                s.resumeUntilSyscall(p, ps, 0);
            } else {
                s.resumeUntilSyscall(p, ps, 0);
            }
        }
        break;
    case SIGCHLD:
        s.debugLog(p, "SIGCHLD");
        s.resumeUntilSyscall(p, ps, SIGCHLD);
        break;
    case SIGSTOP:
        if (ps.newClone) {
            s.debugLog(p, "Expected SIGSTOP in new clone.");
            ps.newClone = false;
            if (ptrace(PTRACE_SETOPTIONS, p, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC)) {
                switch (errno) {
                case ESRCH:
                    logFatalError(s.os, p, "tracesysgood failed - no such process.", p);
                default:
                    logFatalError(s.os, p, "tracesysgood failed %d.", p, errno);
                }
                s.stopMonitoring(p, ps);
                s.resumeWithoutMonitoring(p);
            } else {
                s.resumeUntilSyscall(p, ps, SIGSTOP);
            }
        } else {
            s.resumeUntilSyscall(p, ps, SIGSTOP);
        }
        break;
    default:
        s.debugLog(p, "SIGNAL %d", WSTOPSIG(status));
        logError(s.os, p, "Unexpected signal %d.", WSTOPSIG(status));
        s.resumeUntilSyscall(p, ps, signal);
    }
}

void loop(state_t& s, const struct signalfd_siginfo& siginfo) {
    int status;
    pid_t p = wait(&status);
    if (p == -1) {
        switch (errno) {
        case ECHILD:
            logError(s.os, p, "wait failed due to no children.");
            return;
        case EINTR:
            logError(s.os, p, "Unexpected signal in wait.");
            return;
        default:
            logError(s.os, p, "wait failed (errno = %d).", errno);
            return;
        }
    }

    if (WIFEXITED(status)) {
        auto ip = s.processMap.find(p);
        if (ip == s.processMap.end()) {
            logError(s.os, p, "Unexpected tracee.");
            return;
        }
        process_t& ps = ip->second;
        printExited(s.os, p, WEXITSTATUS(status));
        if (ps.running) {
            s.stopMonitoring(p, ps);
        }
    } else if (WIFSIGNALED(status)) {
        auto ip = s.processMap.find(p);
        if (ip == s.processMap.end()) {
            logError(s.os, p, "Unexpected tracee.");
            return;
        }
        process_t& ps = ip->second;
        printSignaled(s.os, p, WTERMSIG(status));
        if (ps.running) {
            s.stopMonitoring(p, ps);
        }
    } else if (WIFSTOPPED(status)) {
        auto ip = s.processMap.find(p);
        if (ip == s.processMap.end()) {
            logError(s.os, p, "Unexpected tracee.");
            s.resumeWithoutMonitoring(p);
            return;
        }
        process_t& ps = ip->second;
        if (!ps.running) {
            logError(s.os, p, "Unexpected tracee stop event after terminal failure.");
            s.resumeWithoutMonitoring(p);
            return;
        }
        processStopped(s, p, ps, status);
    } else if (WIFCONTINUED(status)) {
        auto ip = s.processMap.find(p);
        if (ip == s.processMap.end()) {
            logError(s.os, p, "Unexpected tracee.");
            s.resumeWithoutMonitoring(p);
            return;
        }
        process_t& ps = ip->second;
        if (!ps.running) {
            logError(s.os, p, "Unexpected tracee continued event after terminal failure.");
            s.resumeWithoutMonitoring(p);
            return;
        }
        logError(s.os, p, "Unexpected continue.");
        s.resumeUntilSyscall(p, ps);
    } else {
        auto ip = s.processMap.find(p);
        if (ip == s.processMap.end()) {
            logError(s.os, p, "Unexpected tracee.");
            s.resumeWithoutMonitoring(p);
            return;
        }
        process_t& ps = ip->second;
        if (!ps.running) {
            logError(s.os, p, "Unexpected wait after terminal failure.");
            s.resumeWithoutMonitoring(p);
            return;
        }
        logError(s.os, p, "Unexpected wait %d.", status);
        s.resumeUntilSyscall(p, ps);
    }
}

}

int btrace(const Params& params) {
    setvbuf(stdout, NULL, _IONBF, 0);
    state_t s(params.debug);

    int stdoutPipe[2];
    if (pipe2(stdoutPipe, 0)) {
        logFatalError(s.os, -1, "Pipe failed (errno = %d).", errno);
        return -1;
    }
    int stderrPipe[2];
    if (pipe2(stderrPipe, 0)) {
        logFatalError(s.os, -1, "Pipe failed (errno = %d).", errno);
        return -1;
    }

    // Launch application
    pid_t p = fork();
    // Child process
    if (p == 0) {
        close(stdoutPipe[0]);
        close(stderrPipe[0]);
        ptrace(PTRACE_TRACEME);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stderrPipe[1], STDERR_FILENO);

        std::vector<char*> execArgs;
        execArgs.reserve(params.args.size() + 1);
        for (auto& a : params.args)
            execArgs.push_back((char*) a.c_str());
        execArgs.push_back(0);
        int r = execve(params.cmd.c_str(), &execArgs[0], params.envp);
        exit(errno);
    }
    close(stdoutPipe[1]);
    close(stderrPipe[1]);
    const int stdoutReadFd = stdoutPipe[0];
    const int stderrReadFd = stderrPipe[0];

    // Create mask that contains currently blocked signals.
    sigset_t mask;
    if (sigemptyset(&mask)) {
        logFatalError(s.os, -1, "Failed to initialize signal mask.");
        kill(p, SIGKILL);
        return -1;
    }
    // Add SIGCHLD
    if (sigaddset(&mask, SIGCHLD)) {
        logFatalError(s.os, -1, "sigaddset(.., SIGCHLD) failed.");
        kill(p, SIGKILL);
        return -1;
    }

    // Block SIGCHLD signals
    if (sigprocmask(SIG_BLOCK, &mask, 0)) {
        logFatalError(s.os, -1, "sigprocmask failed to set signals (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }

    // Create signalfd for listening to signals.
    int sigfd = signalfd(-1, &mask, 0);
    if (sigfd == -1) {
        logFatalError(s.os, -1, "signalfd create failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }

    // Create epollfd to listen to sigfd and child stderr/stdout.
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        logFatalError(s.os, -1, "epoll_create failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }

    // Add
    epoll_event event;
    event.data.fd = sigfd;
    event.events = EPOLLIN;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sigfd, &event)) {
        logFatalError(s.os, -1, "epoll_ctl failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }

    event.data.fd = stdoutReadFd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, stdoutReadFd, &event)) {
        logFatalError(s.os, -1, "epoll_ctl failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }
    event.data.fd = stderrReadFd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, stderrReadFd, &event)) {
        logFatalError(s.os, p, "epoll_ctl failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }

    for (const auto& exe : params.knownExes) {
        s.untracedExes.insert(exe);
    }

    int status;
    int r = waitpid(p, &status, 0);
    if (r == -1) {
        logFatalError(s.os, p, "waitpid failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
    }

    // If execve in child thread succeeds, then next event will be stop with a trap.
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        // Do nothing
    // Otherwise it should be a exit as execve failed.
    } else if (WIFEXITED(status)) {
        logFatalError(s.os, p, "Could not run %s (errno = %d).", params.cmd.c_str(), WEXITSTATUS(status));
        // Quit (child printed error message).
        return -1;
    } else {
        logFatalError(s.os, p, "Unexpected result from fork (status = %d).", status);
        kill(p, SIGKILL);
        return -1;
    }

    if (ptrace(PTRACE_SETOPTIONS, p, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC)) {
        logFatalError(s.os, p, "ptrace(PTRACE_SETOPTIONS, ..) failed (errno = %d).", errno);
        kill(p, SIGKILL);
        return -1;
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
    if (!populateCwdExe(s, p, initCmd)) {
        logFatalError(s.os, p, "Initial process %s untraced.", initCmd.exePath.c_str());
        kill(p, SIGKILL);
        return -1;
    }
    // Resume from trap now that state is setup.
    s.resumeUntilSyscall(p, ps);

    // Run
    while (s.hasAliveProcesses()) {
        int r = epoll_wait(epollfd, &event, 1, -1);
        if (r > 0) {
            int fd = event.data.fd;
            if (fd == sigfd) {
                struct signalfd_siginfo sig;
                ssize_t br = read(sigfd, &sig, sizeof(sig));
                loop(s, sig);
            } else if (fd == stdoutReadFd) {
                char buf[4092];
                ssize_t br = read(stdoutReadFd, buf, sizeof(buf));
                if (br == -1) {
                    logError(s.os, -1, "Failed to read stdout (errno = %d).", errno);
                } else {
                    printOutput(s.os, "stdout", buf, br);
                }
            } else if (fd == stderrReadFd) {
                char buf[4092];
                ssize_t br = read(stderrReadFd, buf, sizeof(buf));
                if (br == -1) {
                    logError(s.os, -1, "Failed to read stdout (errno = %d).", errno);
                } else {
                    printOutput(s.os, "stderr", buf, br);
                }
            } else {
                logFatalError(s.os, -1, "Unexpected filedescriptor %d", fd);
                return -1;
            }
        }
    }
    close(epollfd);
    close(sigfd);
    return s.hasErrors() ? -1 : 0;
}