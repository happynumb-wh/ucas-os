#ifndef _ELF_H
#define _ELF_H
#include <os/string.h>
#include <os/list.h>
#include <pgtable.h>
#include <type.h>
/* 64-bit ELF base types. */
typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef int16_t Elf64_SHalf;
typedef uint64_t Elf64_Off;
typedef int32_t Elf64_Sword;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;
typedef uint16_t Elf64_Section;
typedef Elf64_Half Elf64_Versym;

typedef struct {
  uint64_t a_type;
  union {
      uint64_t a_val;
  } a_un;
} Elf64_auxv_t;

extern uint64_t kload_buffer;

#define LOAD_BUFFER (kload_buffer)

#define AUX_CNT 38

#define AT_NULL		0
#define AT_IGNORE	1
#define AT_EXECFD	2
#define AT_PHDR		3
#define AT_PHENT	4
#define AT_PHNUM	5
#define AT_PAGESZ	6
#define AT_BASE		7
#define AT_FLAGS	8
#define AT_ENTRY	9
#define AT_NOTELF	10
#define AT_UID		11
#define AT_EUID		12
#define AT_GID		13
#define AT_EGID		14
#define AT_CLKTCK	17
#define AT_PLATFORM	15
#define AT_HWCAP	16
#define AT_FPUCW	18
#define AT_DCACHEBSIZE	19
#define AT_ICACHEBSIZE	20
#define AT_UCACHEBSIZE	21
#define AT_IGNOREPPC	22
#define	AT_SECURE	23
#define AT_BASE_PLATFORM 24
#define AT_RANDOM	25
#define AT_HWCAP2	26
#define AT_EXECFN	31
#define AT_SYSINFO	32
#define AT_SYSINFO_EHDR	33
#define AT_L1I_CACHESHAPE	34
#define AT_L1D_CACHESHAPE	35
#define AT_L2_CACHESHAPE	36
#define AT_L3_CACHESHAPE	37


#define EI_MAG0 0 /* e_ident[] indexes */
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_PAD 8

#define ELFMAG0 0x7f /* EI_MAG */
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFMAG "\177ELF"
#define SELFMAG 4

#define ELFCLASSNONE 0 /* EI_CLASS */
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFCLASSNUM 3

#define ELFDATANONE 0 /* e_ident[EI_DATA] */
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_NONE 0 /* e_version, EI_VERSION */
#define EV_CURRENT 1
#define EV_NUM 2

#define ELFOSABI_NONE 0
#define ELFOSABI_LINUX 3

#ifndef ELF_OSABI
#define ELF_OSABI ELFOSABI_NONE
#endif

#define EI_NIDENT 16
typedef struct elf64_hdr
{
    unsigned char e_ident[EI_NIDENT]; /* ELF "magic number" */
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry; /* Entry point virtual address */
    Elf64_Off e_phoff;  /* Program header table file offset */
    Elf64_Off e_shoff;  /* Section header table file offset */
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf64_Half    sh_name;    /* section name */
    Elf64_Half    sh_type;    /* section type */
    Elf64_Xword   sh_flags;    /* section flags */
    Elf64_Addr    sh_addr;    /* virtual address */
    Elf64_Off     sh_offset;    /* file offset */
    Elf64_Xword   sh_size;    /* section size */
    Elf64_Half    sh_link;    /* link to another */
    Elf64_Half    sh_info;    /* misc info */
    Elf64_Xword   sh_addralign;    /* memory alignment */
    Elf64_Xword   sh_entsize;    /* table entry size */
} Elf64_Shdr;

/* These constants define the permissions on sections in the
   program header, p_flags. */
#define PF_X (1 << 0)          /* Segment is executable */
#define PF_W (1 << 1)          /* Segment is writable */
#define PF_R (1 << 2)          /* Segment is readable */
#define PF_MASKOS 0x0ff00000   /* OS-specific */
#define PF_MASKPROC 0xf0000000 /* Processor-specific */

/* Legal values for p_type (segment type).  */

#define PT_NULL 0          /* Program header table entry unused */
#define PT_LOAD 1          /* Dynamic linking information */
#define PT_INTERP 3        /* Loadable program segment */
#define PT_DYNAMIC 2       /* Program interpreter */
#define PT_NOTE 4          /* Auxiliary information */
#define PT_SHLIB 5         /* Reserved */
#define PT_PHDR 6          /* Entry for header table itself */
#define PT_TLS 7           /* Thread-local storage segment */
#define PT_NUM 8           /* Number of defined types */
#define PT_LOOS 0x60000000 /* Start of OS-specific */
#define PT_GNU_EH_FRAME                     \
    0x6474e550 /* GCC .eh_frame_hdr segment \
                */
#define PT_GNU_STACK                                             \
    0x6474e551                  /* Indicates stack executability \
                                 */
#define PT_GNU_RELRO 0x6474e552 /* Read-only after relocation */
#define PT_LOSUNW 0x6ffffffa
#define PT_SUNWBSS 0x6ffffffa   /* Sun Specific segment */
#define PT_SUNWSTACK 0x6ffffffb /* Stack segment */
#define PT_HISUNW 0x6fffffff
#define PT_HIOS 0x6fffffff   /* End of OS-specific */
#define PT_LOPROC 0x70000000 /* Start of processor-specific */
#define PT_HIPROC 0x7fffffff /* End of processor-specific */

/* sh_type */
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0x8fffffff

/* sh_flags */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xF0000000

typedef struct elf64_phdr
{
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;   /* Segment file offset */
    Elf64_Addr p_vaddr;   /* Segment virtual address */
    Elf64_Addr p_paddr;   /* Segment physical address */
    Elf64_Xword p_filesz; /* Segment size in file */
    Elf64_Xword p_memsz;  /* Segment size in memory */
    Elf64_Xword p_align;  /* Segment alignment, file & memory */
} Elf64_Phdr;

typedef struct ELF_info{
    uint64_t text_begin;
    uint64_t phoff;
    uint64_t phent;
    uint64_t phnum;
    uint64_t entry;
    uint64_t edata;
} ELF_info_t;

typedef struct aux_elem
{
    uint64_t id;
    uint64_t val;
}aux_elem_t;

#define LOAD_PHDR_NUM 8
#define MAX_TEST_NUM 8
typedef struct excellent_load
{
    // for debug
    char name[50];
    uint32_t used;
    // base pgdir for the exec and read_only
    uintptr_t base_pgdir;
    // save the data page page
    list_head list;
    // phdr head
    Elf64_Phdr phdr[LOAD_PHDR_NUM];
    int phdr_num;
    // elf head
    ELF_info_t elf;
    int dynamic;
} excellent_load_t;

extern excellent_load_t pre_elf[MAX_TEST_NUM];

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static inline int is_elf_format(unsigned char *binary)
{

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)binary;
    if (ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
        ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr->e_ident[EI_MAG3] == ELFMAG3) {
        return 1;
    }

    return 0;
}

static inline uint64_t get_page_flag(Elf64_Phdr * phdr)
{
    uint64_t flag = _PAGE_ACCESSED;
    if (phdr->p_flags & PF_R)
        flag |= _PAGE_READ;
    if (phdr->p_flags & PF_W)
        flag |= _PAGE_WRITE | _PAGE_DIRTY;
    if (phdr->p_flags & PF_X)
        flag |= _PAGE_EXEC;
    return flag;
}

// for debug print the ehdr message
void debug_print_ehdr(Elf64_Ehdr ehdr);

// for debug print the phdr message
void debug_print_phdr(Elf64_Phdr phdr);


typedef struct Elffile ElfFile;
typedef struct pcb pcb_t;

/* prepare_page_for_kva should return a kernel virtual address */
extern uintptr_t load_elf(
    ElfFile *elf, uintptr_t pgdir, uint64_t *mem_alloc, pcb_t *initpcb , int *dynamic);

extern uintptr_t fat32_load_elf(uint32_t fd, uintptr_t pgdir, uint64_t *file_length, int *dynamic, pcb_t *initpcb);


extern uintptr_t fast_load_elf(const char * filename, uintptr_t pgdir, uint64_t *file_length, pcb_t *initpcb);

extern uintptr_t fast_load_elf_execve(const char * filename, pcb_t *initpcb);


#define ELF_LOAD 1
#define ELF_FREE 0
// for fast load elf
int do_pre_load(const char * filename, int how);
void init_pre_load();
void do_pre_elf_qemu(unsigned char elf_binary[], excellent_load_t *exe_load, uint64_t length);
void do_pre_elf_fat32(fd_num_t fd, excellent_load_t *exe_load);

void do_free_elf(excellent_load_t *exe_load);

int get_free_elf();
int find_name_elf(const char * filename);


// ===================== for dynamic ==========================
#define DYNAMIC_VADDR_PFFSET 0x3000000000
extern uintptr_t load_connector(const char * filename, uintptr_t pgdir);
extern uintptr_t load_connnetor_fix(const char * filename, uintptr_t pgdir);

// ===================== for restore =========================
#define SIZE_RESTORE 8
extern void __restore();




static inline uint32_t set_aux_vec(aux_elem_t *aux_vec, ELF_info_t *elf, uintptr_t file_pointer, uintptr_t random_ptr)
{
    uint32_t index = 0;
#define NEW_AUX_ENT(id0, val0) \
    aux_vec[index].id = id0; \
    aux_vec[index++].val = val0; \
    // 0x21
    NEW_AUX_ENT(0x28, 0);
    NEW_AUX_ENT(0x29, 0);
    NEW_AUX_ENT(0x2a, 0);
    NEW_AUX_ENT(0x2b, 0);
    NEW_AUX_ENT(0x2c, 0);
    NEW_AUX_ENT(0x2d, 0);

    // NEW_AUX_ENT(AT_SYSINFO_EHDR, 0x3fc2dee000);     // 0x21
    NEW_AUX_ENT(AT_HWCAP, 0x112d);                  // 0x10
    NEW_AUX_ENT(AT_PAGESZ, NORMAL_PAGE_SIZE);       // 0x06
    NEW_AUX_ENT(AT_CLKTCK, 0x64);                   // 0x11
    NEW_AUX_ENT(AT_PHDR, elf->text_begin + elf->phoff);               // 0x03
    NEW_AUX_ENT(AT_PHENT, elf->phent);              // 0x04
    NEW_AUX_ENT(AT_PHNUM, elf->phnum);              // 0x05
    NEW_AUX_ENT(AT_BASE, DYNAMIC_VADDR_PFFSET);     // 0x07                       // 0x07
    NEW_AUX_ENT(AT_FLAGS, 0);                       // 0x08
    NEW_AUX_ENT(AT_ENTRY, elf->entry);              // 0x09
    NEW_AUX_ENT(AT_UID, 0);                         // 0x0b
    NEW_AUX_ENT(AT_EUID, 0);                        // 0x0c
    NEW_AUX_ENT(AT_GID, 0);                         // 0x0d
    NEW_AUX_ENT(AT_EGID, 0);                        // 0x0e
    NEW_AUX_ENT(AT_SECURE, 0);                      // 0x17
    NEW_AUX_ENT(AT_RANDOM, random_ptr);             // 0x19
    NEW_AUX_ENT(AT_EXECFN, file_pointer);           // 0x1f
    NEW_AUX_ENT(0, 0);
#undef NEW_AUX_ENT
    return index;
}
#endif  // _ELF_H