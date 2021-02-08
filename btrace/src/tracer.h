#pragma once
#include <string>
#include <vector>
#include <stdio.h>

struct  Params {
    int stdout = -1;
    int stderr = -1;
    FILE* output = ::stdout;

    bool debug = false;
    bool jsonOutput = false;

    std::string cmd;
    std::vector<std::string> args;
    char*const* envp;
};

int btrace(const Params& args);