#ifndef __DEBUG_SHELL_H
#define __DEBUG_SHELL_H

struct exec_info {
    u32 *breakpoints;
    int num_breakpoints;
    int num_steps;
};


#ifdef DEBUG
    extern int shell(void);
    extern void cons_printf(char *fmt, ...);
    extern int cons_getchar(void);
    extern int cons_getsn(void *buf, int size);
#else
    #include <stdio.h>
    #define getsn(a, b) fread((a), 1, (b), stdin)

    #define cons_printf printf
    #define cons_getchar getchar
    #define cons_getsn getsn
#endif

#endif
