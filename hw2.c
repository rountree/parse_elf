#define write_syscall 1

int
_start(void){
    int retval;
    int fd = 1;
    char message[] ="Hello, World!\n";
    unsigned long size = 14;

    asm volatile(
            "movl %[write], %%eax \n "
            "syscall \n "
            : "=a"(retval)
            : "D"(fd), "S"(message), "d"(size), [write] "i"(write_syscall)
            : "rcx", "r11", "cc" );
     return retval;
}



