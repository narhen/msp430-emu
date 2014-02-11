#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include <stdlib.h>

#define u8 uint8_t
#define s8 int8_t

#define u16 uint16_t
#define s16 int16_t

#define u32 uint32_t
#define s32 int32_t

#define u64 uint64_t
#define s64 int64_t

#include <msp430/jumps.h>
#include <msp430/single_operand.h>
#include <msp430/double_operand.h>

#define PC 0
#define SP 1
#define SR 2
#define CG 3

#define _SR_C        0
#define _SR_Z        1
#define _SR_N        2
#define _SR_GIE      3
#define _SR_CPU_OFF  4
#define _SR_OSC_OFF  5
#define _SR_SCG0     6
#define _SR_SCG1     7
#define _SR_V        8

#define SR_C        (1 << _SR_C)
#define SR_Z        (1 << _SR_Z)
#define SR_N        (1 << _SR_N)
#define SR_GIE      (1 << _SR_GIE)
#define SR_CPU_OFF  (1 << _SR_CPU_OFF)
#define SR_OSC_OFF  (1 << _SR_OSC_OFF)
#define SR_SCG0     (1 << _SR_SCG0)
#define SR_SCG1     (1 << _SR_SCG1)
#define SR_V        (1 << _SR_V)

#define AS_REG      0x0
#define AS_IDX      0x1
#define AS_SYM      0x1
#define AS_ABS      0x1
#define AS_INDREG   0x2
#define AS_INDINC   0x3
#define AS_IMM      0x3

#define read_word(addr) (*(u16 *)(u8 *)(memory + (addr)))
#define write_word(addr, val) (*((u16 *)(u8 *)(memory + (addr))) = (u16)(val))

#define read_byte(addr) (*(memory + addr))
#define write_byte(addr, val) (*(memory + addr) = (val))

#define read_bits(val, mask) ((val) & (mask))
#define set_bits(var, mask) ((var) |= (mask))
#define clr_bits(var, mask) ((var) &= (~(mask)))
/* set bit number i in var */
#define set_bit(var, i) (set_bits(var, (1 << i))
#define clr_bit(var, i) (clr_bits(var, (1 << i))
#define read_bit(val, i) (read_bits(val, i))


#define _packed __attribute__((packed))

extern u8 memory[0x10000 + 32];
extern u16 *registers;

extern double curr_time(void);
extern char *get_registers(void);
extern void print_registers(void);
extern void dump_memory(u16 addr, u16 size);

/* increases register by a word (2 bytes) */
static inline u32 inc_reg(int reg)
{
    registers[reg] += 2;

    return registers[reg];
}

static inline u32 dec_reg(int reg)
{
    registers[reg] -= 2;

    return registers[reg];
}

#endif
