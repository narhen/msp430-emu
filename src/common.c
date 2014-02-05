#include <stdio.h>
#include <sys/time.h>
#include <msp430/common.h>

u8 memory[0x10000 + 32];
u16 *registers = (u16 *)&memory[0x10000];

double curr_time(void)
{
    struct timeval  tv;
    gettimeofday(&tv, NULL);

    double time_in_mill = 
             (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0 ; // convert tv_sec & tv_usec to millisecond

    return time_in_mill / 1000.0;
}

void print_registers(void)
{
    printf(" pc: %04x  sp: %04x  sr: %04x  cg: %04x\n", registers[PC], registers[SP],
            registers[SR], registers[CG]);

    printf("r04: %04x r05: %04x ", registers[4], registers[5]);
    printf("r06: %04x r07: %04x\n", registers[6], registers[7]);

    printf("r08: %04x r09: %04x ", registers[8], registers[9]);
    printf("r10: %04x r11: %04x\n", registers[10], registers[11]);

    printf("r12: %04x r13: %04x ", registers[12], registers[13]);
    printf("r14: %04x r15: %04x\n", registers[14], registers[15]);
}
