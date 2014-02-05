#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <msp430/common.h>

int load_elf(char *file)
{
    int i;
    char *strtab = NULL;
    FILE *fp = NULL;
    Elf32_Ehdr header;
    Elf32_Shdr *sections = NULL, *ptr;

    if ((fp = fopen(file, "rb")) == NULL)
        return 0;

    /* read the elf header */
    fread(&header, sizeof(Elf32_Ehdr), 1, fp);

    /* check the magic number is correct */
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
    strtab = malloc(sections[header.e_shstrndx].sh_size + 1);
    fseek(fp, sections[header.e_shstrndx].sh_offset, SEEK_SET);
    fread(strtab, sections[header.e_shstrndx].sh_size, 1, fp);

    /* iterate sectiuon headers and load the appropriate ones into memory */
    ptr = sections;
    for (i = 0; i < header.e_shnum; ++i, ++ptr) {
        if (ptr->sh_type != SHT_PROGBITS)
            continue;

        if (!strcmp(strtab + ptr->sh_name, ".debug_"))
            continue;

        fseek(fp, ptr->sh_offset, SEEK_SET);
        fread(memory + ptr->sh_addr, ptr->sh_addr, 1, fp);
    }

    free(sections);;
    free(strtab);
    fclose(fp);

    return 1;
error:
    fclose(fp);
    return 0;
}
