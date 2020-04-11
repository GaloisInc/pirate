#ifndef RESOURCE_LOADER_H
#define RESOURCE_LOADER_H
int load_resources(int &argc, char *  *&argv);

#define PIRATE_EMPTY_RESOURCE(name) namespace { char empty_##name[0] __attribute__((used,section(".pirate.res." #name))); }
PIRATE_EMPTY_RESOURCE(int)
PIRATE_EMPTY_RESOURCE(bool)
PIRATE_EMPTY_RESOURCE(string)
PIRATE_EMPTY_RESOURCE(milliseconds)
#undef EMPTY_RESOURCE

#endif
