
int run_green(int argc, char** argv);

#ifdef GAPS_DISABLE
int main(int argc, char** argv) {
    return run_green(argc, argv);
}
#endif
