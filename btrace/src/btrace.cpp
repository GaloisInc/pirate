#include "tracer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

static
std::string concatPath(const std::string& p, const char* fname) {
    if (p.back() == '/') {
        return p + fname;
    } else {
        return p + '/' + fname;
    }
}

static
std::string findApp(char* nm) {
    const char* r = strrchr(nm, '/');
    if (r != 0) {
        return std::string(nm);
    }
    const char* path = getenv("PATH");
    while (path) {
        const char* next = strchr(path,':');
        std::string dir;
        if (next) {
            dir = std::string(path, next);
            path = next+1;
        } else {
            dir = std::string(path);
            path = 0;
        }
        if (dir.empty()) continue;
        std::string path = concatPath(dir, nm);
        struct stat buf;
        if (stat(path.c_str(), &buf)) {
            continue;
        }
        bool isExec = buf.st_mode & S_IXOTH;
        if (!isExec) continue;
        return path;
    }
    fprintf(stderr, "Could not find %s.\n", nm);
    exit(-1);
}

void showUsage(FILE* f, const char* exe) {
    fprintf(f, "Usage:\n");
    fprintf(f, "  %s --help: Show help\n", exe);
    fprintf(f, "  %s flags* cmd args\n", exe);
    fprintf(f, "    --json: Emit output in json format\n");
    fprintf(f, "    --verbose|-v: Emit debugging information\n");
    fprintf(f, "    --output path: Path to emit collected information to (stdout is default)\n");
    fprintf(f, "    --stdout path: Path to redirect stdout to (stdout is default)\n");
    fprintf(f, "    --stdout path: Path to redirect stderr to (stderr is default)\n");
    fprintf(f, "    cmd:  Program to trace.\n");
    fprintf(f, "    args: Command line arguments to cmd.\n");
}

FILE* parseFileArg(char* const*& curArg, char* const* endArg) {
    if (curArg == endArg) {
        fprintf(stderr, "Expected output file.\n");
        exit(-1);
    }
    FILE* r = fopen(*curArg, "w");
    if (!r) {
        fprintf(stderr, "Failed to open %s (errno = %d).\n", *curArg, errno);
        exit(-1);
    }
    ++curArg;
    return r;
}

int parseFdArg(char* const*& curArg, char* const* endArg) {
    if (curArg == endArg) {
        fprintf(stderr, "Expected output file.\n");
        exit(-1);
    }
    int r = open(*curArg, O_CREAT | O_WRONLY | O_TRUNC, 00666);
    if (r == -1) {
        fprintf(stderr, "Failed to open %s (errno = %d).\n", *curArg, errno);
        exit(-1);
    }
    ++curArg;
    return r;
}

void parseArgs(Params& params, int argc, char* const* argv, char*const* envp) {
    char* const* curArg = argv + 1;
    char* const* endArg = argv + argc;
    while (curArg < endArg) {
        if (strcmp(*curArg, "--") == 0) {
            ++curArg;
            break;
        }
        if (strcmp(*curArg, "--help") == 0) {
            showUsage(stdout, argv[0]);
            exit(0);
        }
        if (strcmp(*curArg, "--output") == 0) {
            ++curArg;
            params.output = parseFileArg(curArg, endArg);
        } else if (strcmp(*curArg, "--stdout") == 0) {
            ++curArg;
            params.stdout = parseFdArg(curArg, endArg);
        } else if (strcmp(*curArg, "--stderr") == 0) {
            ++curArg;
            params.stderr = parseFdArg(curArg, endArg);
        } else if (strcmp(*curArg, "--json") == 0) {
            ++curArg;
            params.jsonOutput = true;
        } else if ((strcmp(*curArg, "--verbose") == 0) || (strcmp(*curArg, "-v") == 0)) {
            ++curArg;
            params.debug = true;
        } else if (strncmp(*curArg, "--", 2) == 0) {
            fprintf(stderr, "Unknown flag \"%s\" (use \"--help\" for usage).", *curArg);
            exit(-1);
        } else {
            break;
        }
    }
    if (curArg >= endArg) {
        fprintf(stderr, "Missing command to trace.\n\n");
        fprintf(stderr, "For help run `%s --help`.\n", argv[0]);
        exit(-1);
    }
    params.cmd = findApp(*curArg);
    ++curArg;
    params.args.push_back(params.cmd);
    while (curArg < endArg) {
        params.args.push_back(*curArg);
        ++curArg;
    }
    params.envp = envp;
}

int main(int argc, char* const* argv, char*const* envp) {
    Params params;
    parseArgs(params, argc, argv, envp);
    // Close stdin
    close(0);
    // Run btrace
    return btrace(params);
}