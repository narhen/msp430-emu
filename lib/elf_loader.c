#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <msp430/common.h>

static Elf32_Sym *symtbl;
static int symtbl_size;
static char *shstrtab = NULL;
static char *strtab = NULL;

u16 find_sym(char *sym_name)
{
    int i;
    Elf32_Sym *symptr;
    u16 ret;

    symptr = symtbl;
    for (i = 0; i < symtbl_size; ++i, ++symptr) {
        if (!strcmp(strtab + symptr->st_name, sym_name))
            break;
    }

    if (strncmp(strtab + symptr->st_name, sym_name, 4))
        return -1; /* could't find symbol */

    ret = (u16)symptr->st_value;

    return ret;
}

static int load_sym_table(FILE *fp, Elf32_Shdr *sections, int size)
{
    int i;
    Elf32_Shdr *ptr = sections;

    for (i = 0; i < size; ++i, ++ptr)
        if (ptr->sh_type == SHT_SYMTAB)
            break;

    if (ptr->sh_type != SHT_SYMTAB)
        return 0; /* couldn't find any symtab */

    symtbl = malloc(ptr->sh_size);
    fseek(fp, ptr->sh_offset, SEEK_SET);
    fread(symtbl, ptr->sh_size, 1, fp);

    symtbl_size = ptr->sh_size / sizeof(Elf32_Sym);

    return 1;
}

static char *load_strtab(FILE *fp, Elf32_Shdr *entry)
{
    char *ret;

    ret = malloc(entry->sh_size);
    fseek(fp, entry->sh_offset, SEEK_SET);
    fread(ret, entry->sh_size, 1, fp);

    return ret;
}

int load_elf(FILE *fp)
{
    int i;
    Elf32_Ehdr header;
    Elf32_Shdr *sections = NULL, *ptr;
    Elf32_Phdr *pheaders = NULL, *pptr;

    /* read the elf header */
    fread(&header, sizeof(Elf32_Ehdr), 1, fp);

    /* check if the magic number is correct */
    if (strncmp((const char *)header.e_ident, ELFMAG, 4)) {
        fprintf(stderr, "This is not a valid elf 32 file\n");
        goto error;
    }

    /* check for the right architecture */
    if (header.e_machine != 105) {
        fprintf(stderr, "Not an elf for the Texas Instruments "
                "msp430 microcontroller.\n");
        goto error;
    }

    /* read section headers */
    sections = malloc(sizeof(Elf32_Shdr) * header.e_shnum);
    fseek(fp, (unsigned long)header.e_shoff, SEEK_SET);
    fread(sections, sizeof(Elf32_Shdr), header.e_shnum, fp);

    /* read shstrtab */
    shstrtab = malloc(sections[header.e_shstrndx].sh_size + 1);
    fseek(fp, sections[header.e_shstrndx].sh_offset, SEEK_SET);
    fread(shstrtab, sections[header.e_shstrndx].sh_size, 1, fp);

    /* read program headers */
    pheaders = malloc(header.e_phnum * header.e_phentsize);
    fseek(fp, header.e_phoff, SEEK_SET);
    fread(pheaders, header.e_phnum * header.e_phentsize, 1, fp);

    /* find strtab */
    ptr = sections;
    for (i = 0; i < header.e_shnum; ++i, ++ptr) {
        if (!strncmp(shstrtab + ptr->sh_name, ".strtab", 7)) {
            strtab = load_strtab(fp, ptr);
            break;
        }
    }

    /* load program headers */
    pptr = pheaders;
    for (i = 0; i < header.e_phnum; ++i, ++pptr) {
        if (pptr->p_type != PT_LOAD)
            continue;
        fseek(fp, pptr->p_offset, SEEK_SET);
        fread(memory + pptr->p_paddr, pptr->p_filesz, 1, fp);
    }

    if (!load_sym_table(fp, sections, header.e_shnum))
        fprintf(stderr, "Failed to load symbol table\n");

    free(sections);;
    free(pheaders);

    return 1;
error:
    return 0;
}

int elf_is_loaded(void)
{
    return shstrtab != NULL;
}

void clean_elf_stuff(void)
{
    free(symtbl);
    free(shstrtab);
    free(strtab);
}
