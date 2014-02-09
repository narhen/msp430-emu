#ifndef __ELF_LOADER_H
#define __ELF_LOADER_H

#include <msp430/common.h>

extern int load_elf(char *file, u16 *main_addr);

#endif
