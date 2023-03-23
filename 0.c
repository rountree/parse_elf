// Tell the compiler incoming stack alignment is not RSP%16==8 or ESP%16==12
static const char hw[]="Hello, world!\n";
void _start() {
	asm(	"mov $1,%%rdi    # First parameter:   unsigned int fd (aka stderr_fileno).\n\t"
		"mov $14,%%rdx   # Third parameter:   size_t count.\n\t"
		"mov $1, %%rax   # Syscall ID:        1=write\n\t"
		"syscall       #\n\t"
		"mov $42,%%rdi  # First parameter:   return value is 42\n\t"
		"mov $0x3c,%%rax# Syscall ID:        0x3c = _exit()\n\t"
	    	"syscall\n"
		:
		:"S"(hw));		// Puts the address of hw into rsi
}

