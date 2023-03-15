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

/* Commentary:
 *
 * Source taken from https://www.felixcloutier.com/documents/gcc-asm.html
 * "This page is meant to consolidate <GCC's official extended asm syntax> into
 * a form that is consumable by mere mortals."
 *
 * See compiled code at https://godbolt.org/z/b8feEMrhb
 *      I'm using gcc 10.3.1 on tioga.  clang 15.0.1 works there as well.
 *
 * Big picture:  We can write assembly language in dedicated files that are
 * then given to an assembler (e.g., "as") to turn into machine language.  But
 * we can also drop assembly language in to C and C++.
 *
 * Line
 *
 *   01     The <unistd.h> header pulls in the definition of size_t (line 5) and
 *          STDOUT_FILENO (line 20).  It's part of the POSIX specification, not
 *          the C language specification.  Details at
 *              https://en.wikipedia.org/wiki/Unistd.h
 *
 *          Try replacing line 01 with the following:
 *              typedef unsigned int size_t;
 *              #define STDOUT_FILENO 1
 *
 *   02     The <sys/syscall.h> file is here:
 *              /usr/include/sys/syscall.h
 *
 *          See this file for all of the functionality the kernel implements:
 *              /usr/include/asm/unistd_64.h
 *
 *          Here's a simpler approach.
 *              grep -r SYS_write /usr/include
 *                  /usr/include/bits/syscall.h:# define SYS_write __NR_write
 *              grep -r __NR_write /usr/include
 *                  /usr/include/asm/unistd_64.h:#define __NR_write 1
 *
 *          Try replacing line 02 with the following:
 *              #define SYS_write 1
 *
 *          THE IMPORTANT BIT:  We can't call directly into the kernel using
 *          functions.  Instead, we put the system call number we want into
 *          a particular register, the parameters it needs into other
 *          registers, and then execute the assembly language instruction
 *          "syscall".
 *
 *   03     This line intentionally left blank.
 *
 *   04     The original code had an __attribute__((always_inline)) prefacing
 *          the do_write() function.  It's not necessary here, so I've removed
 *          it.  The attribute is a request that the compiler not perform a
 *          function call to execute this function, but rather copy and paste
 *          this function directly into the code that's calling it.
 *
 *          Otherwise, this function returns an "int".
 *
 *   05     
 */


