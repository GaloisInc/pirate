#define R(name) namespace { char empty_##name[0] __attribute__((used,section("pirate_res_" #name))); }

R(bool)
R(int)
R(milliseconds)
R(string)
