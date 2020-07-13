/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include <iostream>
#include <cstring>

#include <getopt.h>

#include "CDRGenerator.hpp"
#include "indent_facet.hpp"

int main(int argc, char* argv[]) {
    int rv = 0, option_index = 0;
    TargetLanguage target = TargetLanguage::UNKNOWN;
    struct option long_options[] = {
        {"target", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    std::ios_base::sync_with_stdio(false);

    while (true) {
        rv = getopt_long(argc, argv, "t:", long_options, &option_index);
        if (rv < 0) {
            break;
        }
        switch (rv) {
            case 't':
                if (strncmp("cpp", optarg, 4) == 0) {
                    target = TargetLanguage::CPP_LANG;
                } else if (strncmp("c", optarg, 2) == 0) {
                    target = TargetLanguage::C_LANG;
                } else {
                    std::cerr << "unknown code generation target " << optarg << std::endl;
                    return 1;
                }
                break;
            default:
                return 1;
        }
    }

    if (target == TargetLanguage::UNKNOWN) {
        std::cerr << "missing required command-line argument --target" << std::endl;
        return 1;
    }

    return parse(std::cin, std::cout, std::cerr, target);
}
