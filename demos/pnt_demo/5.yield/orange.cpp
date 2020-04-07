
int run_orange(int argc, char** argv);

#ifdef GAPS_DISABLE
int main(int argc, char** argv) {
    return run_orange(argc, argv);
}
#endif
