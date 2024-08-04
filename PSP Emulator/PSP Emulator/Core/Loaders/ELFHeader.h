#pragma once

#include <cstdint>

namespace ELF {
enum Type {
    ET_NONE = 0,
    ET_REL = 1,
    ET_EXEC = 2,
    ET_DYN = 3,
    ET_CORE = 4,
    ET_LOPROC = 0xFF00,
    ET_HIPROC = 0xFFFF,
    ET_PSP_PRX = 0xFFA0,
};

enum Machine {
    EM_MIPS = 8
};

#define EV_NONE    0
#define EV_CURRENT 1

#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6
#define EI_PAD     7
#define EI_NIDENT 16

#define R_MIPS_NONE 0
#define R_MIPS_16 1
#define R_MIPS_32 2
#define R_MIPS_26 4
#define R_MIPS_HI16 5
#define R_MIPS_LO16 6

#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EF_MIPS_NOREORDER 0x00000001
#define EF_MIPS_PIC       0x00000002
#define EF_MIPS_CPIC      0x00000004
#define EF_MIPS_XGOT      0x00000008
#define EF_MIPS_UCODE     0x00000010
#define EF_MIPS_ABI2      0x00000020
#define EF_MIPS_DYNAMIC   0x00000040
#define EF_MIPS_32BITMODE 0x00000100
#define EF_MIPS_ABI_MASK  0x0000f000
#define EF_MIPS_ABI_O32   0x00001000
#define EF_MIPS_ABI_O64   0x00002000
#define E_MIPS_ABI_EABI32 0x00003000
#define E_MIPS_ABI_EABI64 0x00004000
#define EF_MIPS_MACH_MASK 0x00ff0000
#define EF_MIPS_MACH_PSP  0x00a20000
#define EF_MIPS_ARCH_MASK 0xf0000000
#define EF_MIPS_ARCH_1    0x00000000
#define EF_MIPS_ARCH_2    0x10000000
#define EF_MIPS_ARCH_3    0x20000000
#define EF_MIPS_ARCH_4    0x30000000
#define EF_MIPS_ARCH_5    0x40000000
#define EF_MIPS_ARCH_32   0x50000000
#define EF_MIPS_ARCH_64   0x60000000
#define EF_MIPS_ARCH_32R2 0x70000000
#define EF_MIPS_ARCH_64R2 0x80000000

#define SHN_UNDEF          0
#define SHN_LORESERVE 0xFF00
#define SHN_LOPROC    0xFF00
#define SHN_HIPROC    0xFF1F
#define SHN_ABS       0xFFF1
#define SHN_COMMON    0xFFF2
#define SHN_HIRESERVE 0xFFFF

#define SHT_NULL            0
#define SHT_PROGBITS        1
#define SHT_SYMTAB          2
#define SHT_STRTAB          3
#define SHT_RELA            4
#define SHT_HASH            5
#define SHT_DYNAMIC         6
#define SHT_NOTE            7
#define SHT_NOBITS          8
#define SHT_REL             9
#define SHT_SHLIB          10
#define SHT_DYNSYM         11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7FFFFFFF
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xFFFFFFFF

#define SHT_PSPREL 0x700000a0

enum ElfSectionFlags {
    SHF_WRITE = 0x1,
    SHF_ALLOC = 0x2,
    SHF_EXECINSTR = 0x4,
    SHF_MERGE = 0x10,
    SHF_STRINGS = 0x20,
    SHF_MASKPROC = 0xF0000000,
};

// Symbol binding 
#define STB_LOCAL   0 
#define STB_GLOBAL  1 
#define STB_WEAK    2 
#define STB_LOPROC 13 
#define STB_HIPROC 15 

// Symbol types
#define STT_NOTYPE   0
#define STT_OBJECT   1
#define STT_FUNC     2
#define STT_SECTION  3
#define STT_FILE     4
#define STT_LOPROC  13
#define STT_HIPROC  15

// Undefined name
#define STN_UNDEF 0

// Relocation types
#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GOT32     3
#define R_386_PLT32     4
#define R_386_COPY      5
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8
#define R_386_GOTOFF    9
#define R_386_GOTPC    10

// Segment types
#define PT_NULL             0
#define PT_LOAD             1
#define PT_DYNAMIC          2
#define PT_INTERP           3
#define PT_NOTE             4
#define PT_SHLIB            5
#define PT_PHDR             6
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7FFFFFFF

// Custom segment types
#define PT_PSPREL1 0x700000A0
#define PT_PSPREL2 0x700000A1
#define SHT_PRXREL 0x700000A0

// Segment flags
#define PF_X 1
#define PF_W 2
#define PF_R 4

// Dynamic Array Tags
#define DT_NULL              0
#define DT_NEEDED            1
#define DT_PLTRELSZ          2
#define DT_PLTGOT            3
#define DT_HASH              4
#define DT_STRTAB            5
#define DT_SYMTAB            6
#define DT_RELA              7
#define DT_RELASZ            8
#define DT_RELAENT           9
#define DT_STRSZ            10
#define DT_SYMENT           11
#define DT_INIT             12
#define DT_FINI             13
#define DT_SONAME           14
#define DT_RPATH            15
#define DT_SYMBOLIC         16
#define DT_REL              17
#define DT_RELSZ            18
#define DT_RELENT           19
#define DT_PLTREL           20
#define DT_DEBUG            21
#define DT_TEXTREL          22
#define DT_JMPREL           23
#define DT_LOPROC   0x70000000
#define DT_HIPROC   0x7FFFFFFF

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;


// ELF file header
struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
};

// Section header
struct Elf32_Shdr {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
};

// Segment header
struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

// Symbol table entry
struct Elf32_Sym {
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
};

#define ELF32_ST_BIND(i)   ((i)>>4)
#define ELF32_ST_TYPE(i)   ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

// Relocation entries
struct Elf32_Rel {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
};

struct Elf32_Rela {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
    Elf32_Sword r_addend;
};

#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8 )+(unsigned char)(t))
}