#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <msp430/common.h>

static u16 find_main(FILE *fp, Elf32_Shdr *sections, int size, char *strtab)
{
    int i;
    Elf32_Shdr *ptr = sections;
    Elf32_Sym *symtbl, *symptr;
    u16 ret;

    for (i = 0; i < size; ++i, ++ptr) {
        if (ptr->sh_type == SHT_SYMTAB)
            break;
    }

    if (ptr->sh_type != SHT_SYMTAB)
        return -1; /* couldn't find any symtab */

    symtbl = malloc(ptr->sh_size);
    fseek(fp, ptr->sh_offset, SEEK_SET);
    fread(symtbl, ptr->sh_size, 1, fp);

    symptr = symtbl;
    for (i = 0; i < ptr->sh_size / sizeof(Elf32_Sym); ++i, ++symptr) {
        if (!strncmp(strtab + symptr->st_name, "main", 4))
            break;
    }

    if (strncmp(strtab + symptr->st_name, "main", 4))
        return -1; /* could't find main */

    ret = (u16)symptr->st_value;
    free(symtbl);

    return ret;
}

static char *load_strtab(FILE *fp, Elf32_Shdr *entry)
{
    char *ret;

    ret = malloc(entry->sh_size);
    fseek(fp, entry->sh_offset, SEEK_SET);
    fread(ret, entry->sh_size, 1, fp);

    return ret;
}

int load_elf(char *file, u16 *main_addr)
{
    int i;
    char *shstrtab = NULL, *strtab = NULL;
    FILE *fp = NULL;
    Elf32_Ehdr header;
    Elf32_Shdr *sections = NULL, *ptr;
    Elf32_Phdr *pheaders = NULL, *pptr;

    if ((fp = fopen(file, "rb")) == NULL)
        return 0;

    /* read the elf header */
    fread(&header, sizeof(Elf32_Ehdr), 1, fp);

    /* check if the magic number is correct */
    if (!strcmp((const char *)header.e_ident, ELFMAG)) {
        fprintf(stderr, "'%s' is not a valid elf 32 file\n", file);
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

    /* read strtab */
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

    /* find address of main */
    if (strtab != NULL) {
        *main_addr = find_main(fp, sections, header.e_shnum, strtab);
        free(strtab);
    } else
        *main_addr = -1;

    /* load program headers */
    pptr = pheaders;
    for (i = 0; i < header.e_phnum; ++i, ++pptr) {
        if (pptr->p_type != PT_LOAD)
            continue;
        fseek(fp, pptr->p_offset, SEEK_SET);
        fread(memory + pptr->p_paddr, pptr->p_filesz, 1, fp);
    }

    free(sections);;
    free(shstrtab);
    free(pheaders);
    fclose(fp);

    return 1;
error:
    fclose(fp);
    return 0;
}
