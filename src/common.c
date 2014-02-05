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