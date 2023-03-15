/* parse_elf.c
 *
 * References:
 * [1] elf(5) Linux man page 2017-09-15
 * [2] System V Application Binary Interface Edition 4.1, March 18, 1997
 *      https://refspecs.linuxbase.org/elf/gabi41.pdf
 * [3] System V Application Binary Interface DRAFT 24 April 2001
 *      https://refspecs.linuxbase.org/elf/gabi4+/contents.html
 * [4] System V Application Binary Interface AMD64 Architecture Processor Supplement
 *      Draft Version 0.99.6, July 2, 2012
 *      https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf
 * [5] readelf(1) Linux man page 2022-04-25
 * [6] cpprefernce
 *      https://en.cpprefernce.com
 */

#include <stdio.h>      // printf(3), fprintf(3)
#include <getopt.h>     // getopt_long(3)
#include <stdlib.h>     // exit(3), malloc(3)
#include <string.h>     // strlen(3)
#include <assert.h>     // assert(3)
#include <sys/types.h>  // open(2), fstat(2)
#include <sys/stat.h>   // open(2), fstat(2)
#include <fcntl.h>      // open(2)
#include <unistd.h>     // fstat(2)
#include <sys/mman.h>   // mmap(2)
#include <elf.h>

static char *pathname;                  // Name of the file to be parsed
static unsigned char const *map_addr;   // Location of the memory map of the file

[[noreturn]] void
print_help(){
    printf("Usage:  parse_elf [-h|-v]\n");
    printf("        parse_elf <file>\n");
    printf("\n");
    printf("Options:\n");
    printf("    -h      --help      Print this message and exit.\n");
    printf("    -v      --version   Print version information and exit.\n");
    printf("\n");
    exit(0);
}

[[noreturn]] void
print_version(){
    printf("parse_elf v0.01\n");
    exit(0);
}

void
parse_options( int argc, char **argv ){
    int c;
    int option_index=0;
    static struct option long_options[] = {
        {"help",    no_argument,    0, 'h' },
        {"version", no_argument,    0, 'v' },
        {0,         0,              0, 0 }};
    while(1){
        c = getopt_long( argc, argv, "hv", long_options, &option_index );
        if( -1 == c ){
            break;
        }
        switch(c){
            case 'h': print_help();      break;
            case 'v': print_version();   break;
            default:
                      fprintf(stderr, "%s:%s:%d getopt_long returned unknown character code %#x.\n",
                              __FILE__, __func__, __LINE__, c);
                      exit(-1);
        }

    }
    if( optind == argc ){
        fprintf(stderr, "%s:%s:%d No filename specified.\n",
            __FILE__, __func__, __LINE__);
        print_help();
    }else if( optind + 1 < argc ){
        fprintf(stderr, "%s:%s:%d Too many filenames specified.\n",
            __FILE__, __func__, __LINE__);
        print_help();
    }else if( optind + 1 == argc ){
        pathname = calloc( strlen(argv[optind]) + 1, sizeof( char ) );
        assert( NULL != pathname );
        strncpy( pathname, argv[optind], strlen(argv[optind]) );
    }else{
        fprintf(stderr, "%s:%s:%d optind=%d, argc=%d, not sure how we got here.\n",
            __FILE__, __func__, __LINE__, optind, argc);
        exit(-1);
    }
    // If we don't have a pathname, we shouldn't get here.
    assert(pathname);
}

void
cleanup(){
    free(pathname);
}

void
map_file(){
    int fd, rc;
    struct stat s;

    // 1. Get a valid file descriptor.
    fd = open( pathname, O_RDONLY );
    assert( -1 != fd );

    // 2. Get the size of the file.
    rc = fstat( fd, &s );
    assert( -1 != rc );

    // 3. Map the file.
    map_addr = mmap(
            NULL,           // Allow the OS to pick the location of the map.
            s.st_size,      // File size in bytes.
            PROT_READ,      // Map may not be modified.
            MAP_PRIVATE,    // Map not shared with other processes.
            fd,             // File descriptor.
            0);             // Offset into the file to start mapping.
    assert( MAP_FAILED != map_addr );

    assert(    0x7f == map_addr[0]
            &&  'E' == map_addr[1]
            &&  'L' == map_addr[2]
            &&  'F' == map_addr[3]);
}

void
parse_elf_header(){
    size_t offset = 0;
    Elf64_Ehdr *e = (Elf64_Ehdr *)map_addr;
    printf("Elf Header\n\n");

    // Magic number
    printf("%6s %12s %6s %20s %6s %12s\n", "Offset", "Name", "Value", "Meaning", "Size", "Type");
    printf("%#06zx %12s %#6x %20s %6zu %12s\n", offset, "EI_MAG0", e->e_ident[0], "Magic Number 0", sizeof(uint8_t), "uint8_t"); offset += sizeof(e->e_ident[0]);
    printf("%#06zx %12s %6c %20s %6zu %12s\n", offset, "EI_MAG1", e->e_ident[1], "Magic Number 1", sizeof(uint8_t), "uint8_t"); offset += sizeof(e->e_ident[1]);
    printf("%#06zx %12s %6c %20s %6zu %12s\n", offset, "EI_MAG2", e->e_ident[2], "Magic Number 2", sizeof(uint8_t), "uint8_t"); offset += sizeof(e->e_ident[2]);
    printf("%#06zx %12s %6c %20s %6zu %12s\n", offset, "EI_MAG3", e->e_ident[3], "Magic Number 3", sizeof(uint8_t), "uint8_t"); offset += sizeof(e->e_ident[3]);

    // Class
    printf("%#06zx %12s %6u %20s %6zu %12s\n",
            offset,
            "Class",
            e->e_ident[4],
            e->e_ident[4] == ELFCLASSNONE ? "Class" :
            e->e_ident[4] == ELFCLASS32   ? "32-bit architecture" :
            e->e_ident[4] == ELFCLASS64   ? "64-bit architecture" :
            "Invalid class",
            sizeof(uint8_t),
            "uint8_t");
    offset += sizeof(e->e_ident[3]);



}

int
main( [[maybe_unused]] int argc, [[maybe_unused]] char **argv ){
    parse_options( argc, argv );
    map_file();
    parse_elf_header();
    cleanup();
    return 0;
}


