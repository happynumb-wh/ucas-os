#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 4)
#define TASK_INFO_LOC (OS_SIZE_LOC - 4)
#define TASK_NUM_LOC (TASK_INFO_LOC - 2)
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa
#define BOOT_MEM_LOC 0x7c00
#define OS_MEM_LOC 0x50201000
#define TASK_TEXT_OFFSET 0x1000

#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

#define TASK_NAME_MAXLEN 16
typedef struct {
    char name[TASK_NAME_MAXLEN];   // Fixed task name
    uint64_t entrypoint;
    uint64_t offset;  // physical address in image
    uint32_t filesz;
    uint32_t memsz;
} task_info_t;

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr);
static uint64_t get_entrypoint(Elf64_Ehdr ehdr);
static uint32_t get_filesz(Elf64_Phdr phdr);
static uint32_t get_memsz(Elf64_Phdr phdr);
static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr);
static void write_padding(FILE *img, int *phyaddr, int new_phyaddr);
static void write_img_info(int nbytes_kernel, task_info_t *taskinfo,
                           short tasknum, FILE *img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock main) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int tasknum = nfiles - 2;
    int nbytes_kernel = 0;
    int phyaddr = 0;
    FILE *fp = NULL, *img = NULL;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* initialize task info */
    task_info_t *taskinfo = (tasknum <= 0) ? NULL : \
        (task_info_t *)malloc(tasknum * sizeof(task_info_t));
    if (tasknum > 0 && taskinfo == NULL) {
        error("Error: malloc for taskinfo array failed!\n");
    } else if (taskinfo != NULL) {
        bzero(taskinfo, tasknum * sizeof(task_info_t));
    }

    /* open the image file */
    img = fopen(IMAGE_FILE, "w");
    assert(img != NULL);

    /* for each input file */
    for (int fidx = 0; fidx < nfiles; ++fidx) {

        int taskidx = fidx - 2;
        if (taskidx >= 0) {
            taskinfo[taskidx].offset = phyaddr;
        }

        /* open input file */
        fp = fopen(*files, "r");
        assert(fp != NULL);

        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (int ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            if (phdr.p_type != PT_LOAD) continue;

            /* write segment to the image */
            write_segment(phdr, fp, img, &phyaddr);

            /* update nbytes_kernel */
            if (strcmp(*files, "main") == 0) {
                nbytes_kernel += get_filesz(phdr);
            }

            /* update task info */
            if (taskidx >= 0) {
                taskinfo[taskidx].filesz += get_filesz(phdr);
                taskinfo[taskidx].memsz += get_memsz(phdr);
            }
        }

        /* write padding bytes */
        if (strcmp(*files, "bootblock") == 0) {
            write_padding(img, &phyaddr, SECTOR_SIZE);
        }

        /* write task info */
        if (taskidx >= 0) {
            strncpy(taskinfo[taskidx].name, *files, TASK_NAME_MAXLEN);
            taskinfo[taskidx].entrypoint = get_entrypoint(ehdr);
        }

        fclose(fp);
        files++;
    }
    write_img_info(nbytes_kernel, taskinfo, tasknum, img);

    if (taskinfo != NULL) {
        free(taskinfo);
    }
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    int ret;

    ret = fread(ehdr, sizeof(*ehdr), 1, fp);
    assert(ret == 1);
    assert(ehdr->e_ident[EI_MAG1] == 'E');
    assert(ehdr->e_ident[EI_MAG2] == 'L');
    assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    int ret;

    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    ret = fread(phdr, sizeof(*phdr), 1, fp);
    assert(ret == 1);
    if (options.extended == 1) {
        printf("\tsegment %d\n", ph);
        printf("\t\toffset 0x%04lx", phdr->p_offset);
        printf("\t\tvaddr 0x%04lx\n", phdr->p_vaddr);
        printf("\t\tfilesz 0x%04lx", phdr->p_filesz);
        printf("\t\tmemsz 0x%04lx\n", phdr->p_memsz);
    }
}

static uint64_t get_entrypoint(Elf64_Ehdr ehdr)
{
    return ehdr.e_entry;
}

static uint32_t get_filesz(Elf64_Phdr phdr)
{
    return phdr.p_filesz;
}

static uint32_t get_memsz(Elf64_Phdr phdr)
{
    return phdr.p_memsz;
}

static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr)
{
    if (phdr.p_memsz != 0 && phdr.p_type == PT_LOAD) {
        /* write the segment itself */
        /* NOTE: expansion of .bss should be done by kernel or runtime env! */
        if (options.extended == 1) {
            printf("\t\twriting 0x%04lx bytes\n", phdr.p_filesz);
        }
        fseek(fp, phdr.p_offset, SEEK_SET);
        while (phdr.p_filesz-- > 0) {
            fputc(fgetc(fp), img);
            (*phyaddr)++;
        }
    }
}

static void write_padding(FILE *img, int *phyaddr, int new_phyaddr)
{
    if (options.extended == 1 && *phyaddr < new_phyaddr) {
        printf("\t\twrite 0x%04x bytes for padding\n", new_phyaddr - *phyaddr);
    }

    while (*phyaddr < new_phyaddr) {
        fputc(0, img);
        (*phyaddr)++;
    }
}

static void write_img_info(int nbytes_kernel, task_info_t *taskinfo,
                           short tasknum, FILE * img)
{
    /* write bootloader signature */
    char signature;
    fseek(img, BOOT_LOADER_SIG_OFFSET, SEEK_SET);
    signature = BOOT_LOADER_SIG_1;
    fwrite(&signature, sizeof(signature), 1, img);
    fseek(img, BOOT_LOADER_SIG_OFFSET + 1, SEEK_SET);
    signature = BOOT_LOADER_SIG_2;
    fwrite(&signature, sizeof(signature), 1, img);

    /* write kernel size */
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&nbytes_kernel, sizeof(nbytes_kernel), 1, img);
    if (options.extended == 1) {
        printf("os_size: 0x%04x bytes\n", nbytes_kernel);
    }

    /* write number of tasks */
    fseek(img, TASK_NUM_LOC, SEEK_SET);
    fwrite(&tasknum, sizeof(tasknum), 1, img);
    if (options.extended == 1) {
        printf("tasknum: %d tasks\n", tasknum);
    }

    /* write position of app info struct */
    int info_offset = taskinfo[tasknum - 1].offset + taskinfo[tasknum - 1].filesz;
    fseek(img, TASK_INFO_LOC, SEEK_SET);
    fwrite(&info_offset, sizeof(info_offset), 1, img);
    if (options.extended == 1) {
        printf("task info: located at image phyaddr 0x%04x\n", info_offset);
    }

    /* write task info */
    int taskinfo_len = tasknum * sizeof(task_info_t);
    if (taskinfo_len > 0) {
        fseek(img, info_offset, SEEK_SET);
        fwrite(taskinfo, sizeof(task_info_t), tasknum, img);
    }
    if (options.extended == 1) {
        printf("task info: write 0x%04x bytes\n", taskinfo_len);
    }
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
