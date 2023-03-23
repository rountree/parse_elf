all: parse_elf 0

parse_elf: parse_elf.c Makefile
	clang-13 -std=c2x -O3 -Wall -Wextra -Werror -o parse_elf parse_elf.c

0: 0.c Makefile
	clang-13 -S 0.c
	clang-13 -g -s -static -nostartfiles -nodefaultlibs -nolibc -nostdlib -Os -o 0 0.s



