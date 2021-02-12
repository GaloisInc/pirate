# Pirate Linker Errors

This extension does two things. Primarily, it makes a best effort at running the linking steps for a Pirate application in order to display helpful errors that are not caught by normal C/C++ IDE extensions.  For instance, if a programmer tries to call a function that requires a certain capability from an enclave that does not posess that capability, then that error is caught at link-time and would typically not be shown dynamically in VS Code.  This fixes that.

Second, it provides a simple CodeLens that shows basic capability information for functions open in the editor.  Since the annotations could be in a different file, it is often helpful to propogate that information in such a way that the programmer can see it directly.

The extension requires a working CMake configuration for your project, and makes a best effort at finding CMake, make, and your compilers automatically.  If it fails in some capacity, you can set specific paths in the extension settings.
