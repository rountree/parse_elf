all:
	clang -std=c2x -O3 -Wall -Wextra -Werror -o parse_elf parse_elf.c

hw: hw.c
	gcc -o hw hw.c

