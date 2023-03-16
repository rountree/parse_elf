all:
	clang -std=c2x -O3 -Wall -Wextra -Werror -o parse_elf parse_elf.c

hw: hw.c
	gcc -o hw hw.c

hw2: hw2.c
	gcc -Wall -Wextra -nostartfiles -nodefaultlibs -nostdlib -o hw2 hw2.c

