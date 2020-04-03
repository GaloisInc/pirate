#pragma once

extern "C" {

struct pirate_resource_param {
    const char *key, *value;
};

struct pirate_resource {
    const char *name;
    pirate_resource_param *params;
    void *object;
};

}
