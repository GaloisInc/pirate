
int run_orange(int argc, char** argv);

#ifndef GAPS_ENABLE
int main(int argc, char** argv) {
    return run_orange(argc, argv);
}
#endif
