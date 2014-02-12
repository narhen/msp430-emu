#include <stdio.h>

#include <msp430/common.h>
#include <debug_shell.h>


static inline void opc_components(u16 instruction, u16 *opcode, u16 *sreg,
        u16 *ad, u16 *bw, u16 *as, u16 *dreg)
{
    *opcode = read_bits(instruction, 0xf000) >> 12;
    *sreg = read_bits(instruction, 0x0f00) >> 8;
    *ad = read_bits(instruction, 0x80) >> 7;
    *bw = read_bits(instruction, 0x40) >> 6;
    *as = read_bits(instruction, 0x30) >> 4;
    *dreg = read_bits(instruction, 0xf);
}

static u32 get_addr(u16 addr_mode, u16 reg, u16 *write_back, u32 *val)
{
    u32 addr = 0;
    *write_back = 1;

    switch (addr_mode) {
        case 0:
            /* register */
            addr = 0x10000 + (reg << 1);
            if (reg == CG)
                registers[CG] = 0;
            *val = read_word(addr);
            break;
        case 1:
            if (reg != CG) {
                addr = read_word(registers[PC]);
                if (reg != SR)
                    addr += registers[reg];
                addr &= 0xffff;
                *val = read_word(addr);
                inc_reg(PC);
            } else {
                registers[CG] = 1;
                addr = 0x10000 + (reg << 1);
                *val = registers[reg];
            }
            break;
        case 2:
            if (reg == CG) {
                registers[CG] = 2;
                addr = 0x10000 + (reg << 1);
                *val = registers[reg];
            } else if (reg == SR) {
                registers[CG] = 4;
                reg = CG;
                addr = 0x10000 + (reg << 1);
                *val = registers[reg];
            } else {
                addr = 0x10000 + (reg << 1);
                *val = read_word(registers[reg]);
            }
            break;
        case 3:
            if (reg != PC) {
                if (reg == CG)
                    registers[CG] = 0xffff;
                else if (reg == SR)
                    registers[CG] = 8;
                addr = 0x10000 + (registers[reg] << 1);
                *val = read_word(registers[reg]);
                inc_reg(reg);
            } else {
                addr = registers[PC];
                *val = read_word(addr);
                inc_reg(PC);
                *write_back = 0;
            }
            break;
    }

    return addr;
}

static inline void set_sr_flags(u16 dval, int byte)
{
    int n = !!read_bits(dval, 1 << (((!byte + 1) * 8) - 1));
    int z = !read_bits(dval, (1 << ((!byte + 1) * 8)) - 1);

    registers[SR] = (registers[SR] & ~(SR_N|SR_Z)) | ((n << _SR_N)|(z << _SR_Z));
}

static inline void set_sr_flags_add(u32 sval, u32 dval, int byte)
{
    int n = !!read_bits(dval, 1 << (((!byte + 1) * 8) - 1));
    int z = !read_bits(dval, (1 << ((!byte + 1) * 8)) - 1);
    int c = !!read_bits(dval, 1 << (((!byte + 1) * 8)));
    int v = 0; // how to set this?

    registers[SR] = (registers[SR] & ~(SR_N|SR_Z)) | ((n << _SR_N)|(z << _SR_Z));
    registers[SR] = (registers[SR] & ~(SR_C|SR_V)) | ((c << _SR_C)|(v << _SR_V));
}

static inline void set_sr_flags_and(u32 dval, int byte)
{
    int n = !!read_bits(dval, 1 << (((!byte + 1) * 8) - 1));
    int z = !read_bits(dval, (1 << ((!byte + 1) * 8)) - 1);
    int c = !z;
    int v = 0;

    registers[SR] = (registers[SR] & ~(SR_N|SR_Z)) | ((n << _SR_N)|(z << _SR_Z));
    registers[SR] = (registers[SR] & ~(SR_C|SR_V)) | ((c << _SR_C)|(v << _SR_V));
}

static inline void set_sr_flags_xor(u32 sval, u32 dval, int byte)
{
    int n = !!read_bits(dval, 1 << (((!byte + 1) * 8) - 1));
    int z = !read_bits(dval, (1 << ((!byte + 1) * 8)) - 1);
    int c = !z;
    int v = read_bits(dval, 1 << (((!byte + 1) * 8) - 1)) ^
        read_bits(sval, 1 << (((!byte + 1) * 8) - 1));

    registers[SR] = (registers[SR] & ~(SR_N|SR_Z)) | ((n << _SR_N)|(z << _SR_Z));
    registers[SR] = (registers[SR] & ~(SR_C|SR_V)) | ((c << _SR_C)|(v << _SR_V));
}

static u32 mov(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    return sval;
}

static u32 add(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif

    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = (sval & mask) + (dval & mask);
    set_sr_flags_add(sval, ret, bw);

    return ret & mask;
}

static u32 addc(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif

    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = (sval & mask) + (dval & mask) + read_bits(registers[SR], SR_C);
    set_sr_flags_add(sval, ret, bw);

    return ret & mask;
}

static u32 subc(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = dval - sval - 1 + read_bits(registers[SR], SR_C);

    set_sr_flags_add(sval, ret, bw);

    return ret & mask;
}

static u32 sub(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = dval - sval;

    set_sr_flags_add(sval, dval, bw);

    return ret & mask;
}

static u32 cmp(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = dval - sval;

    set_sr_flags_add(sval, dval, bw);

    return ret & mask;
}

/* XXX IMPLEMENT THIS CORRECTLY */
static u32 dadd(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif

    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = (sval & mask) + (dval & mask);
    set_sr_flags_add(sval, ret, bw);

    return ret & mask;
}

static u32 bit(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = sval & dval;

    set_sr_flags_and(dval, bw);

    return ret & mask;
}

static u32 bic(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = ~sval & dval;

    return ret & mask;
}

static u32 bis(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = sval | dval;

    return ret & mask;
}

static u32 xor(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = sval ^ dval;

    set_sr_flags_xor(sval, dval, bw);

    return ret & mask;
}

static u32 and(u32 sval, u32 dval, u16 bw)
{
#ifdef DEBUG2
    cons_printf("%s\n", __FUNCTION__);
#endif
    u32 ret;
    u32 mask = (1 << ((!bw + 1) * 8)) - 1;

    ret = sval & dval;

    set_sr_flags_and(dval, bw);

    return ret & mask;
}

static u32 (*call_table[])(u32, u32, u16) = {
    mov,
    add,
    addc,
    subc,
    sub,
    cmp,
    dadd,
    bit,
    bic,
    bis,
    xor,
    and,
};

void handle_double(u16 instruction)
{
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb;
    u32 sval, dval;

    opc_components(instruction, &opcode, &sreg, &ad, &bw, &as, &dreg);
    get_addr(as, sreg, &wb, &sval);
    u32 daddr = get_addr(ad, dreg, &wb, &dval);

    dval = call_table[opcode - 4](sval, dval, bw);

    if (wb) {
        if (bw) {
            dval &= 0xff;
            write_byte(daddr, dval);
        } else
            write_word(daddr, dval);
    }
}
