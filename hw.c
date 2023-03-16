#include <unistd.h>
#include <sys/syscall.h>
int
do_write( int fd, void *ptr, size_t size ){
    int retval;
    asm volatile(
            "movl %[write], %%eax \n "
            "syscall"
            : "=a"(retval)
            : "D"(fd), "S"(ptr), "d"(size), [write] "i"(SYS_write)
            : "rcx", "r11", "cc");
    return retval;
}

char message[] ="Hello, World!\n";

int
main(){
    do_write(STDOUT_FILENO, message, sizeof(message)-1);
}

