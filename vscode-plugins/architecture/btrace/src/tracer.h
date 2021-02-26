#pragma once
#include <string>
#include <vector>
#include <stdio.h>

struct  Params {
    /** Path to pattern file */
    std::vector<const char*> knownExes;
    bool debug = false;

    const char* cmd;
    std::vector<std::string> args;
    char*const* envp;
};

int btrace(const Params& args);