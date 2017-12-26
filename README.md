# Kernel 'syscall' interception

Disclaimer: a kernel module to substitute originl 'open()' system call with forged one. (Restores original one on driver unload.)

Notice: an address of originl 'open()' is not computed - should be parsed out from  '/boot/System.map-xxxxx'.

STATUS: BEING EVALUATED
