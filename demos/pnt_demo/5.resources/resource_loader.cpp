#include "resource.h"
#include "resource_loader.h"
#include "wrapped_array.h"

#include <getopt.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#define DECLARE_KNOWN_RESOURCE(name) pirate_resource __attribute__((weak)) *name##_start, *name##_end
#define KNOWN_RESOURCE(name) wrap_array(name##_start, name##_end)
DECLARE_KNOWN_RESOURCE(string_resources);
DECLARE_KNOWN_RESOURCE(bool_resources);
DECLARE_KNOWN_RESOURCE(int_resources);
DECLARE_KNOWN_RESOURCE(milliseconds_resources);

enum class HasArg { None, Required, Optional, };

struct Handler {
    char const* name;
    HasArg hasArg;
    std::string documentation;
    std::function<bool(char const*)> callback;

    Handler(
        char const* name, HasArg hasArg,
        std::string documentation,
        std::function<bool(char const*)> callback
    ) : name(name), hasArg(hasArg), documentation(documentation), callback(callback) {}
};

namespace {
    int getopt_hasarg(HasArg x) {
        switch (x) {
            case HasArg::None:     return no_argument;
            case HasArg::Required: return required_argument;
            case HasArg::Optional: return optional_argument;
        }
    }

    int getopt_loader(int &argc, char **&argv, std::vector<Handler> const& handlers) {
        
        std::vector<option> longopts;
        for (auto const& h : handlers) {
            longopts.push_back({h.name, getopt_hasarg(h.hasArg)});
        }
        longopts.push_back({});

        int ch;
        int optlongindex;
        opterr = 0; // disable error messages on stderr
        while (-1 != (ch = getopt_long(argc, argv, "", longopts.data(), &optlongindex))) {
            switch (ch) {
                default: abort();
                case '?': return -1;       
                case 0: if (handlers[optlongindex].callback(optarg)) { return -1; }
            }
        }

        // remove flag arguments from program argument array
        argc -= optind;
        argv += optind;

        return 0;
    }

    template<typename T>
    void for_each_param(pirate_resource const& R, T f) {
        if (R.params) {
            for (auto x = R.params; x->key; x++) {
                f(x->key, x->value);
            }
        }
    }
}

int load_resources(int &argc, char **&argv) {

    std::vector<Handler> handlers;

    // Install string resource handlers
    for (auto const& res : KNOWN_RESOURCE(string_resources)) {
        
        std::string doc = "";
        for_each_param(res, [&](char const* key, char const* val) {
            if (0 == strcmp("doc", key)) {
                doc = val;
            }
        });

        auto obj = static_cast<std::string*>(res.object);

        handlers.emplace_back(
            res.name,
            HasArg::Required,
            doc,
            [obj](const char *arg){ *obj = arg; return false; }
        );
    }

    // Install bool resource handlers
    for (auto const& res : KNOWN_RESOURCE(bool_resources)) {

        std::string doc = "";
        for_each_param(res, [&](char const* key, char const* val) {
            if (0 == strcmp("doc", key)) {
                doc = val;
            }
        });

        auto obj = static_cast<bool*>(res.object);

        handlers.emplace_back(
            res.name,
            HasArg::Optional,
            doc,
            [obj](char const* arg){
                if (nullptr == arg || 0 == strcasecmp(arg, "yes")) {
                    *obj = true;
                } else if (0 == strcasecmp(arg, "no")) {
                    *obj = false;
                } else {
                    return true;
                }
                return false;
            }
        );
    }

    // Install int resource handlers
    for (auto const& res : KNOWN_RESOURCE(int_resources)) {
 
        std::string doc = "";
        auto base = 0;
        for_each_param(res, [&](char const* key, char const* val) {
            if (0 == strcmp("base", key)) {
                base = atoi(val);
                if (base < 2 || base > 36) abort(); // bad base parameter
            } else if (0 == strcmp("doc", key)) {
                doc = val;
            }
        });

        auto obj = static_cast<int*>(res.object);

        handlers.emplace_back(
            res.name,
            HasArg::Required,
            doc,
            [obj, base](char const* arg){
                try {
                    *obj = std::stoi(arg, /*pos*/0, base);
                } catch (std::invalid_argument const& e) {
                    return true;
                } catch (std::out_of_range const& e) {
                    return true;
                }
                return false;
            }
        );
    }

    // Install milliseconds resource handlers
    for (auto const& res : KNOWN_RESOURCE(milliseconds_resources)) {

        std::string doc = "";
        for_each_param(res, [&](char const* key, char const* val) {
            if (0 == strcmp("doc", key)) {
                doc = val;
            }
        });

        auto obj = static_cast<std::chrono::milliseconds*>(res.object);

        handlers.emplace_back(
            res.name,
            HasArg::Required,
            doc,
            [obj](char const* arg){
                try {
                    auto value = std::stoll(arg);
                    *obj = std::chrono::milliseconds(value);
                } catch (std::invalid_argument const& e) {
                    return true;
                } catch (std::out_of_range const& e) {
                    return true;
                }
                return false;
            }
        );
    }

    // install help handlers
    handlers.emplace_back(
        "help",
        HasArg::None,
        "Show available settings",
        [&](char const* arg) {
            std::cerr << "Available command-line options:" << std::endl;
            for (auto const& h : handlers) {
                std::string arg;
                arg += "--";
                arg += h.name;

                switch (h.hasArg) {
                    case HasArg::None: break;
                    case HasArg::Required: arg += "=ARG"; break;
                    case HasArg::Optional: arg += "[=ARG]"; break;
                }

                std::cerr << "  " << std::setw(30) << std::left << arg << " "
                          << h.documentation << std::endl;
            }
            return true;
        }
    );

    return getopt_loader(argc, argv, handlers);
}
