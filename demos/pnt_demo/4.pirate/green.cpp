
int run_green(int argc, char** argv);

#ifndef GAPS_ENABLE
int main(int argc, char** argv) {
    return run_green(argc, argv);
}
#endif
