#include "resource.h"
#include "resource_loader.h"
#include "wrapped_array.h"

#include <getopt.h>

#include <algorithm>
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

#define DECLARE_KNOWN_RESOURCE(name) pirate_resource __attribute__((weak)) *name##_start, *name##_end
#define KNOWN_RESOURCE(name) wrap_array(name##_start, name##_end)
DECLARE_KNOWN_RESOURCE(string_resources);
DECLARE_KNOWN_RESOURCE(bool_resources);
DECLARE_KNOWN_RESOURCE(int_resources);
DECLARE_KNOWN_RESOURCE(milliseconds_resources);

int load_resources(int &argc, char **&argv) {

    std::vector<option> longopts;
    std::vector<std::function<void()>> callbacks;
    std::vector<std::string> documentation;

    // Error condition flag set by argument callback handlers
    bool err = 0;

    // Install string resource handlers
    for (auto const& res : KNOWN_RESOURCE(string_resources)) {
        longopts.push_back({res.name, required_argument});

        std::string doc = "";
        if (nullptr != res.params) {
            for (pirate_resource_param *cursor = res.params; nullptr != cursor->key; cursor++) {
                if (0 == strcmp("doc", cursor->key)) {
                    doc = cursor->value;
                }
            }
        }
        documentation.emplace_back(std::move(doc));

        auto obj = static_cast<std::string*>(res.object);
        callbacks.push_back([obj](){ *obj = optarg; });
    }

    // Install bool resource handlers
    for (auto const& res : KNOWN_RESOURCE(bool_resources)) {
        longopts.push_back({res.name, optional_argument});

        std::string doc = "";
        if (nullptr != res.params) {
            for (pirate_resource_param *cursor = res.params; nullptr != cursor->key; cursor++) {
                if (0 == strcmp("doc", cursor->key)) {
                    doc = cursor->value;
                }
            }
        }
        documentation.emplace_back(std::move(doc));

        auto obj = static_cast<bool*>(res.object);
        callbacks.push_back([obj, &err](){
            if (nullptr == optarg || 0 == strcasecmp(optarg, "yes")) {
                *obj = true;
            } else if (0 == strcasecmp(optarg, "no")) {
                *obj = false;
            } else {
                err = true;
            }
         });
    }

    // Install int resource handlers
    for (auto const& res : KNOWN_RESOURCE(int_resources)) {
        longopts.push_back({res.name, required_argument});
 
        std::string doc = "";
        auto base = 0;
        if (nullptr != res.params) {
            for (pirate_resource_param *cursor = res.params; nullptr != cursor->key; cursor++) {
                if (0 == strcmp("base", cursor->key)) {
                    base = atoi(cursor->value);
                    if (base < 2 || base > 36) return -1; // bad base parameter
                } else if (0 == strcmp("doc", cursor->key)) {
                    doc = cursor->value;
                }
            }
        }
        documentation.emplace_back(std::move(doc));

        auto obj = static_cast<int*>(res.object);
        callbacks.push_back([obj, base, &err](){
            char *end;
            auto value = strtol(optarg, &end, base);
            if (nullptr != end && end - optarg == strlen(optarg)) {
                *obj = value;
            } else {
                err = 1;
            }
        });
    }

    // Install milliseconds resource handlers
    for (auto const& res : KNOWN_RESOURCE(milliseconds_resources)) {
        longopts.push_back({res.name, required_argument});

        std::string doc = "";
        if (nullptr != res.params) {
            for (pirate_resource_param *cursor = res.params; nullptr != cursor->key; cursor++) {
                if (0 == strcmp("doc", cursor->key)) {
                    doc = cursor->value;
                }
            }
        }
        documentation.emplace_back(std::move(doc));

        auto obj = static_cast<std::chrono::milliseconds*>(res.object);
        callbacks.push_back([obj, &err](){
            char *end;
            auto value = strtoll(optarg, &end, 10);
            if (nullptr != end && end - optarg == strlen(optarg)) {
                *obj = std::chrono::milliseconds(value);
            } else {
                err = 1;
            }
        });
    }

    // install help handlers
    longopts.push_back({"help", no_argument});
    callbacks.push_back([&]() {
        std::cerr << "Available command-line options:" << std::endl;
        for (size_t i = 0; i < longopts.size()-1; i++) {
            std::cerr << "  --" << std::setw(20) << std::left << longopts[i].name << " " << documentation[i] << std::endl;
        }
        err = 1;
    });

    // getopt_long requires a zero-terminator
    longopts.push_back(option());

    int ch;
    int optlongindex;
    opterr = 0; // disable error messages on stderr
    while (-1 != (ch = getopt_long(argc, argv, "", longopts.data(), &optlongindex))) {
        switch (ch) {
            default: abort();
            case '?': return -1;       
            case 0: callbacks[optlongindex]();
        }
        if (err) { return -1; }
    }

    // remove flag arguments from program argument array
    argc -= optind;
    argv += optind;

    return 0;
}
