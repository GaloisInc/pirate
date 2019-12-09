# platform-gaps

The script [platform.py](/platform-gaps/platform.py) will combine several
executables into a single executable. Each executable is associated with
a unique identifier. When the resulting program is run, it will execute
one of the input executables based on the matching unique identifier.

TODO: encrypt the executables stored in the generated program

Usage:

```
gcc -o hello hello.c
gcc -o world world.c
./platform.py a.out .secretfile hello foo world bar
```
