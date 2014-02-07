#ifdef DEBUG
#include <stdio.h>
#endif

#include <msp430/common.h>

static int jne_nz(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return !read_bits(registers[SR], SR_Z);
}

static int jeq_z(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return read_bits(registers[SR], SR_Z);
}

static int jnc(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return !read_bits(registers[SR], SR_C);
}

static int jc(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return read_bits(registers[SR], SR_C);
}

static int _jn(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return !read_bits(registers[SR], SR_N);
}

static int jge(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return !(read_bits(registers[SR], SR_N) ^ (read_bits(registers[SR], SR_V) >> 6));
}

static int jl(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return read_bits(registers[SR], SR_N) ^ (read_bits(registers[SR], SR_V) >> 6);
}

static int jmp(void)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    return 1;
}

static int (*call_table[])(void) = {
    jne_nz,
    jeq_z,
    jnc,
    jc,
    _jn,
    jge,
    jl,
    jmp,
};

void handle_jumps(u16 instruction)
{
    u16 type, do_jump, offset;

    type = read_bits(instruction, 0xfc00) >> 10;
    type = (type & 0x7) + ((type & 0x8) >> 3);

    do_jump = call_table[type - 1]();

    if (do_jump) {
        offset = read_bits(instruction, 0x3ff);
        /* sign extend */
        if (offset & 0x200)
            offset |= 0xfc00;

        offset <<= 1;

        registers[PC] += offset;
    }
}
