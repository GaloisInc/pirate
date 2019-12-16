# platform-gaps

The script [platform.py](/platform-gaps/platform.py) will combine several
executables into a single executable. Each executable is associated with
a unique identifier. The executables are encrypted using the associated
unique identifier. When the resulting program is run, it will execute
one of the input executables based on the matching unique identifier.

Usage:

```
gcc -o hello hello.c
gcc -o world world.c
./platform.py a.out .secretfile hello foo world bar
```
