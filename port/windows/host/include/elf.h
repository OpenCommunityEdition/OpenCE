/*
ELF.H

The ELF file's structures and values the guest image's loader reads
(port/android/host/host_loader.c), which Windows has no header for.
*/

#ifndef WIN64_HOST_ELF_H
#define WIN64_HOST_ELF_H

#include <stdint.h>

#define EI_NIDENT 16
#define EI_CLASS 4
#define EI_DATA 5
#define ELFMAG "\177ELF"
#define SELFMAG 4
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_EXEC 2
#define EV_CURRENT 1
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef struct
{
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type, e_machine;
	uint32_t e_version, e_entry, e_phoff, e_shoff, e_flags;
	uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type, e_machine;
	uint32_t e_version;
	uint64_t e_entry, e_phoff, e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
	uint32_t p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} Elf32_Phdr;

typedef struct
{
	uint32_t p_type, p_flags;
	uint64_t p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align;
} Elf64_Phdr;

#endif
