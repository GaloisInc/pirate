# PNT Demo Project Pirate initial version

This is an adaption of the NGC 6 month demo code to partition the
demo into two executables.

The project can be built by building the top-level cmake file
with `-DGAPS_DEMOS=On`, or standalone with the commands below.

## Standalone building

If you would like to build the PNT demo by itself, you need to
first install the pirate LLVM compiler and libraries, and then
run `cmake` with pointers to where the pirate tools are installed.
For example, the script below when run from `demos/pnt_demo/get_optversion`
will create a build directory, configure and build the PNT
demo, and then run both binaries using pipes.

```
mkdir build
cd build
CXXFLAGS="-I $HOME/opt/pirate/include" \
LDFLAGS="-L$HOME/opt/pirate/lib" \
cmake .. -DCMAKE_CXX_COMPILER=$HOME/opt/pirate/clang++
make
LD_LIBRARY_PATH=$HOME/opt/pirate/lib ../scripts/run_both.sh
```

## Building within VSCode

