// Functions for printing events in JSON format to stream.
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/writer.h"
#include "rapidjson/filewritestream.h"
#include <vector>

/**
 * Write JSON for a (possibly fatal message).
 *
 * If fatal is set, then this indicates the process will no
 * longer be monitored and additional messages about that process
 * are not expected to appear.  Additonal non-fatal messages may
 * appear if btrace unexpectedly receives a message.
 */
template<typename S>
void printMessage(S& os, bool fatal, pid_t p, const char* msg) {
    rapidjson::Writer<S> writer(os);
    writer.StartObject();
    writer.Key("tag");
    writer.String(fatal ? "fatal" : "message");
    if (p != -1) {
        writer.Key("pid");
        writer.Uint64(p);
    }
    writer.Key("message");
    writer.String(msg);
    writer.EndObject();
    os.Put('\n');
    os.Flush();
}

template<typename S>
void logError(S& os, pid_t p, const char* fmt, ...) {
    char* s;
    va_list vl;
    va_start(vl, fmt);
    int cnt = vasprintf(&s, fmt, vl);
    va_end(vl);
    if (cnt == -1) {
        const char* msg = "internal error: log failed.";
        printMessage(os, false, p, msg);
        return;
    }

    printMessage(os, false, p, s);
    free(s);
}

template<typename S>
void logFatalError(S& os, pid_t p, const char* fmt, ...) {
    char* s;
    va_list vl;
    va_start(vl, fmt);
    int cnt = vasprintf(&s, fmt, vl);
    va_end(vl);
    if (cnt == -1) {
        const char* msg = "format error when printing fatal message.\n";
        printMessage(os, true, p, msg);
        return;
    }
    printMessage(os, true, p, s);
    free(s);
}

/**
 * Write a print message element.
 *
 * If fatal is set, then this indicates the process will no
 * longer be monitored and additional messages about that process
 * are not expected to appear.  Additonal non-fatal messages may
 * appear if btrace unexpectedly receives a message.
 */
template<typename S>
void printOutput(S& os, const char* tag, const char* msg, ssize_t cnt) {
    rapidjson::Writer<S> writer(os);
    writer.StartObject();
    writer.Key("tag");
    writer.String(tag);
    writer.Key("value");
    writer.String(msg, cnt);
    writer.EndObject();
    os.Put('\n');
    os.Flush();
}

/** Notify that the process ended with the given exit code. */
template<typename S>
void printExited(S& os, pid_t p, uint64_t exitCode) {
    rapidjson::Writer<S> writer(os);
    writer.StartObject();
    writer.Key("tag");
    writer.String("exit");
    writer.Key("pid");
    writer.Uint64(p);
    writer.Key("code");
    writer.Uint64(exitCode);
    writer.EndObject();
    os.Put('\n');
    os.Flush();
}

template<typename S>
void printSignaled(S& os, pid_t p, uint64_t signal) {
    rapidjson::Writer<S> writer(os);
    writer.StartObject();
    writer.Key("tag");
    writer.String("signal");
    writer.Key("pid");
    writer.Uint64(p);
    writer.Key("signal");
    writer.Uint64(signal);
    writer.EndObject();
    os.Put('\n');
    os.Flush();
}

template<typename S>
void printClone(S& os, pid_t parent, pid_t child) {
    rapidjson::Writer<S> writer(os);
    writer.StartObject();
    writer.Key("tag");
    writer.String("clone");
    writer.Key("parent");
    writer.Uint64(parent);
    writer.Key("child");
    writer.Uint64(child);
    writer.EndObject();
    os.Put('\n');
    os.Flush();
}

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
    /**
     * Path for actual executable (empty if could not be retrieved).
      */
    std::string exePath;
    /** Path for current working directory (empty if could not be retrieved). */
    std::string cwdPath;
};

template<typename S>
void printExecve(S& os, pid_t p, const execve_t& e, bool traced) {
    rapidjson::Writer<S> writer(os);
    writer.StartObject();
    writer.Key("tag");
    writer.String("execve");
    writer.Key("pid");
    writer.Uint64(p);
    writer.Key("cmd");
    writer.String(e.cmd);
    if (!e.args.empty()) {
        writer.Key("args");
        writer.StartArray();
        for (const std::string& a : e.args) {
            writer.String(a);
        }
        writer.EndArray();
    }
    if (!e.cwdPath.empty()) {
        writer.Key("cwd");
        writer.String(e.cwdPath);
    }
    if (!e.exePath.empty()) {
        writer.Key("exe");
        writer.String(e.exePath);
    }
    if (!e.env.empty()) {
        writer.Key("env");
        writer.StartArray();
        for (const std::string& a : e.env) {
            writer.String(a);
        }
        writer.EndArray();
    }
    writer.Key("traced");
    writer.Bool(traced);
    writer.EndObject();
    os.Put('\n');
    os.Flush();
}