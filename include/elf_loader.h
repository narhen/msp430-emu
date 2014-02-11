#ifndef __ELF_LOADER_H
#define __ELF_LOADER_H

#include <msp430/common.h>

extern u16 find_sym(char *sym_name);
extern int load_elf(FILE *file);
extern int elf_is_loaded(void);
extern void clean_elf_stuff(void);

#endif
