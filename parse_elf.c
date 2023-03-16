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
#include <stdint.h>     // uint64_t and friends
#include <inttypes.h>   // PRIu64 and friends
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
    Elf64_Ehdr *e = (Elf64_Ehdr *)map_addr;
    printf("Elf Header\n\n");

    // Magic number
    printf("%6s %24s %18s %35s %6s %12s\n", "Offset", "Name", "Value", "Meaning", "Size", "Type");
    printf("%6s %24s %18s %35s %6s %12s\n", "======", "========================", "==================", "===================================", "======", "===========");
    printf("%#06zx %24s %#18x %35s %6zu %12s\n", 0x0000UL, "Magic 0", e->e_ident[0], "Magic Number 0", sizeof(uint8_t), "uint8_t");
    printf("%#06zx %24s %18c %35s %6zu %12s\n",  0x0001UL, "Magic 1", e->e_ident[1], "Magic Number 1", sizeof(uint8_t), "uint8_t");
    printf("%#06zx %24s %18c %35s %6zu %12s\n",  0x0002UL, "Magic 2", e->e_ident[2], "Magic Number 2", sizeof(uint8_t), "uint8_t");
    printf("%#06zx %24s %18c %35s %6zu %12s\n",  0x0003UL, "Magic 3", e->e_ident[3], "Magic Number 3", sizeof(uint8_t), "uint8_t");

    // Class
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0004UL,
            "Class",
            e->e_ident[4],
            e->e_ident[4] == ELFCLASSNONE ? "No class" :
            e->e_ident[4] == ELFCLASS32   ? "32-bit architecture" :
            e->e_ident[4] == ELFCLASS64   ? "64-bit architecture" :
            "Invalid class",
            sizeof(uint8_t),
            "uint8_t");

    // Endianess
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0005UL,
            "Data",
            e->e_ident[5],
            e->e_ident[5] == ELFDATANONE ? "Unknown data format" :
            e->e_ident[5] == ELFDATA2LSB ? "Two's complement, little-endian" :
            e->e_ident[5] == ELFDATA2MSB ? "Two's complement, big endian" :
            "Invalid data",
            sizeof(uint8_t),
            "uint8_t");

    // Version
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0006UL,
            "Version",
            e->e_ident[6],
            e->e_ident[6] == EV_NONE ? "Invalid version" :
            e->e_ident[6] == EV_CURRENT ? "Current version" :
            "Invalid version",
            sizeof(uint8_t),
            "uint8_t");

    // ABI
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0007UL,
            "OS ABI",
            e->e_ident[7],
            e->e_ident[7] == ELFOSABI_NONE ? "SYSV" :
            e->e_ident[7] == ELFOSABI_SYSV ? "SYSV" :
            e->e_ident[7] == ELFOSABI_HPUX ? "HPUX" :
            e->e_ident[7] == ELFOSABI_NETBSD ? "NETBSD" :
            e->e_ident[7] == ELFOSABI_LINUX ? "Linux" :
            e->e_ident[7] == ELFOSABI_SOLARIS ? "Solaris" :
            e->e_ident[7] == ELFOSABI_IRIX ? "Irix" :
            e->e_ident[7] == ELFOSABI_FREEBSD ? "FreeBSD" :
            e->e_ident[7] == ELFOSABI_TRU64 ? "Tru64" :
            e->e_ident[7] == ELFOSABI_ARM ? "Arm" :
            e->e_ident[7] == ELFOSABI_STANDALONE ? "Standalone" :
            "Invalid ABI",
            sizeof(uint8_t),
            "uint8_t");

    // ABI version
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0008UL,
            "ABI Version",
            e->e_ident[8],
            e->e_ident[8] == 0 ? "Valid ABI version" :
            "Invalid ABI version",
            sizeof(uint8_t),
            "uint8_t");

    // Padding
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0009UL,
            "ABI Version",
            (unsigned int)e->e_ident[9] +
            (unsigned int)e->e_ident[10] +
            (unsigned int)e->e_ident[11] +
            (unsigned int)e->e_ident[12] +
            (unsigned int)e->e_ident[13] +
            (unsigned int)e->e_ident[14] +
            (unsigned int)e->e_ident[15],
            "Sum of padding (expected 0)",
            (size_t)7,
            "n/a");


    // Object file type
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0010UL,
            "File type",
            e->e_type,
            e->e_type == ET_NONE ? "Unknown type" :
            e->e_type == ET_REL  ? "A relocatable file" :
            e->e_type == ET_EXEC ? "An executable file" :
            e->e_type == ET_DYN  ? "An shared object" :
            e->e_type == ET_CORE ? "A core file" :
            "Invalid file type",
            sizeof(uint16_t),
            "uint16_t");

    // Machine type
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0012UL,
            "Machine type",
            e->e_machine,
            e->e_machine == EM_NONE ? "Unknown machine" :
            e->e_machine == EM_M32 ? "AT&T WE 32100" :
            e->e_machine == EM_SPARC ? "Sun Microsystems SPARC" :
            e->e_machine == EM_386 ? "Intel 80386" :
            e->e_machine == EM_68K ? "Motorola 68000" :
            e->e_machine == EM_88K ? "Motorola 88000" :
            e->e_machine == EM_860 ? "Intel 80860" :
            e->e_machine == EM_MIPS ? "MIPS RS3000 (big endian only)" :
            e->e_machine == EM_PARISC ? "HP/PA" :
            e->e_machine == EM_SPARC32PLUS ? "SPARC with enhanced instruction set" :
            e->e_machine == EM_PPC ? "PowerPC" :
            e->e_machine == EM_PPC64 ? "PowerPC 64-bit" :
            e->e_machine == EM_S390 ? "IBM S/390" :
            e->e_machine == EM_ARM ? "Advanced RISC Machines" :
            e->e_machine == EM_SH ? "Renesas SuperH" :
            e->e_machine == EM_SH ? "Renesas SuperH" :
            e->e_machine == EM_SPARCV9 ? "SPARC v9 64-bit" :
            e->e_machine == EM_IA_64 ? "Intel Itanium" :
            e->e_machine == EM_X86_64 ? "AMD x86-64" :
            e->e_machine == EM_VAX ? "DEC Vax" :
            "Invalid machine type",
            sizeof(uint16_t),
            "uint16_t");

    // File version (?)
    printf("%#06zx %24s %18u %35s %6zu %12s\n",
            0x0014UL,
            "File version",
            e->e_version,
            e->e_version == EV_NONE ? "Invalid version" :
            e->e_version == EV_CURRENT ? "Current version" :
            "Really invalid version",
            sizeof(uint32_t),
            "uint32_t");

    // Entry point
    printf("%#06zx %24s %#18"PRIx64" %35s %6zu %12s\n",
            0x0018UL,
            "Execution entry point",
            e->e_entry,
            "Virtual address",
            sizeof(uint64_t),
            "uint64_t");

    // Program header offset
    printf("%#06zx %24s %#18"PRIx64" %35s %6zu %12s\n",
            0x0020UL,
            "Program header offset",
            e->e_phoff,
            "Program header table starts here",
            sizeof(uint64_t),
            "uint64_t");

    // Section header offset
    printf("%#06zx %24s %#18"PRIx64" %35s %6zu %12s\n",
            0x0028UL,
            "Section header offset",
            e->e_shoff,
            "Section header table starts here",
            sizeof(uint64_t),
            "uint64_t");

    // Flags
    printf("%#06zx %24s %#18"PRIx32" %35s %6zu %12s\n",
            0x0030UL,
            "Processor-specific flags",
            e->e_flags,
            "None defined",
            sizeof(uint32_t),
            "uint32_t");

    // Elf header size
    printf("%#06zx %24s %#18"PRIx16" %35s %6zu %12s\n",
            0x0034UL,
            "ELF header size",
            e->e_ehsize,
            "Size of this ELF header",
            sizeof(uint16_t),
            "uint16_t");

    // Size of single program header entry
    printf("%#06zx %24s %#18"PRIx16" %35s %6zu %12s\n",
            0x0036UL,
            "Program hdr entry size",
            e->e_phentsize,
            "Size of single program header entry",
            sizeof(uint16_t),
            "uint16_t");

    // Number of program header entries
    printf("%#06zx %24s %#18"PRIx16" %35s %6zu %12s\n",
            0x0038UL,
            "Program hdr entry count",
            e->e_phnum,
            "Number of program header entries",
            sizeof(uint16_t),
            "uint16_t");

    // Size of single section header entry
    printf("%#06zx %24s %#18"PRIx16" %35s %6zu %12s\n",
            0x003aUL,
            "Section hdr entry size",
            e->e_shentsize,
            "Size of single section header entry",
            sizeof(uint16_t),
            "uint16_t");

    // Number of section header entries
    printf("%#06zx %24s %#18"PRIx16" %35s %6zu %12s\n",
            0x003cUL,
            "Section hdr entry count",
            e->e_shnum,
            "Number of section header entries",
            sizeof(uint16_t),
            "uint16_t");

    // Section header index for the string table.
    printf("%#06zx %24s %#18"PRIx16" %35s %6zu %12s\n",
            0x003eUL,
            "Section hdr str idx",
            e->e_shnum,
            e->e_shnum == SHN_UNDEF ? "No string table present" :
            e->e_shnum == SHN_XINDEX ? "Extended index used" :
            "String table section haeder index",
            sizeof(uint16_t),
            "uint16_t");

    printf("\n\n");
}

void
parse_program_headers(){
    Elf64_Ehdr *e = (Elf64_Ehdr *)map_addr;
    Elf64_Phdr *ph = (Elf64_Phdr*)(map_addr+(e->e_phoff));

    printf("Program headers\n");
    printf("\tStart = %#"PRIx64", Count = %#"PRIx16", Size (each)=%#"PRIx16"\n\n",
            e->e_phoff, e->e_phnum, e->e_phentsize);

    // Program header index
    printf("%6s %19s %5s %15s %15s %15s %15s %15s %15s\n",
            "index","type","perms","offset", "vaddr", "paddr", "filesz", "memsz", "align");
    printf("%6s %19s %5s %15s %15s %15s %15s %15s %15s\n",
        "======","==================","=====","===============","===============","===============","===============","===============","===============");
    for(uint16_t i=0; i<e->e_phnum; i++, ph++){

        //     index        type perms       offset       vaddr        paddr        filesz       memsz        align
        printf("%#6"PRIx16" %19s %3c%1c%1c %#15"PRIx64" %#15"PRIx64" %#15"PRIx64" %#15"PRIx64" %#15"PRIx64" %#15"PRIx64"\n",
                i,                                                      // index
                ph->p_type == PT_NULL ? "NULL" :                        // type
                ph->p_type == PT_LOAD ? "LOAD" :
                ph->p_type == PT_DYNAMIC ? "DYNAMIC" :
                ph->p_type == PT_INTERP ? "INTERP" :
                ph->p_type == PT_NOTE ? "NOTE" :
                ph->p_type == PT_SHLIB ? "SHLIB" :
                ph->p_type == PT_PHDR ? "PHDR" :
                ph->p_type >= PT_LOPROC && ph->p_type <= PT_HIPROC ? "Processor-specific" :
                ph->p_type == PT_GNU_STACK ? "GNU_STACK" :
                "Invalid type",
                (ph->p_flags & PF_R) ? 'r' : ' ',                         // flags
                (ph->p_flags & PF_W) ? 'w' : ' ',
                (ph->p_flags & PF_X) ? 'x' : ' ',
                ph->p_offset,                                           // offset
                ph->p_vaddr,                                            // vaddr
                ph->p_paddr,                                            // paddr
                ph->p_filesz,                                           // filesz
                ph->p_memsz,                                            // memsz
                ph->p_align                                             // align
                );
    }
}

int
main( [[maybe_unused]] int argc, [[maybe_unused]] char **argv ){
    parse_options( argc, argv );
    map_file();
    parse_elf_header();
    parse_program_headers();
    cleanup();
    return 0;
}


