# process-vm-device

This kernel module creates a character device. The character device allows
one reader process and one writer process to communicate using the
[process_vm](https://linux.die.net/man/2/process_vm_writev) interface.
The data moves directly between the address spaces of the two processes,
without passing through kernel space.

`process_vm_readv` and `process_vm_writev` are system calls. The functionality
is not available to other parts of the kernel or kernel modules. You will need
to apply the kernel patch
[ksys_process_vm](/devices/process-vm-device/ksys_process_vm.patch) in order to
compile the character device.
