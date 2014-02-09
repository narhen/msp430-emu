#include <stdio.h>

#include <msp430/common.h>


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

static u32 get_addr(u16 addr_mode, u16 reg, u16 *write_back, u16 *val)
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
            if (reg == CG)
                registers[CG] = 2;
            else if (reg == SR) {
                registers[CG] = 4;
                reg = CG;
            }
            addr = 0x10000 + (reg << 1);
            *val = read_word(registers[reg]);
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
    int z = !dval;

    registers[SR] = (registers[SR] & ~(SR_N|SR_Z)) | ((n << _SR_N)|(z << _SR_Z));
}

void mov(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, tmp;
    u32 daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &tmp);

    //printf("saddr: 0x%x, daddr: 0x%x\n", saddr, daddr);

    if (!wb)
        return;

    if (!bw)
        write_word(daddr, sval);
    else
        write_byte(daddr, sval & 0xff);
}

void add(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    sval = read_word(saddr);
    //printf("saddr: 0x%x (%x), daddr: 0x%x\n", saddr, sval, daddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval += sval;
        write_word(daddr, dval);

        if (tmp + sval > 0xffff)
            set_bits(registers[SR], SR_V|SR_C);
        else
            clr_bits(registers[SR], SR_V|SR_C);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval += sval;
        write_byte(daddr, (u8)dval);

        if (dval > 0xff)
            set_bits(registers[SR], SR_V|SR_C);
        else
            clr_bits(registers[SR], SR_V|SR_C);
    }

    set_sr_flags(dval, bw);
}

void addc(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval += sval + read_bits(registers[SR], SR_C);
        write_word(daddr, dval);

        if (tmp + sval > 0xffff)
            set_bits(registers[SR], SR_V|SR_C);
        else
            clr_bits(registers[SR], SR_V|SR_C);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval += sval + read_bits(registers[SR], SR_C);
        write_byte(daddr, (u8)dval);

        if (dval > 0xff)
            set_bits(registers[SR], SR_V|SR_C);
        else
            clr_bits(registers[SR], SR_V|SR_C);
    }

    set_sr_flags(dval, bw);
}

void subc(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg, wb;
    s16 sval, dval, tmp;
    u32 saddr, daddr, c;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, (u16 *)&sval);
    daddr = get_addr(ad, dreg, &wb, (u16 *)&dval);

    if (!wb)
        return;

    c = read_bits(registers[SR], SR_C);
    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval -= sval - c;
        write_word(daddr, dval);

        if (tmp - sval < 0)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);

        if (sval >= 0 && tmp < 0 && tmp - sval - c >= 0)
            set_bits(registers[SR], SR_V);
        else
            clr_bits(registers[SR], SR_V);
    } else {
        sval = read_byte(saddr);
        tmp = dval = read_byte(daddr);
        dval -= sval - c;
        write_byte(daddr, (u8)dval);

        s8 *s = (s8 *)&sval;
        s8 *t = (s8 *)&tmp;

        if (tmp - sval < 0)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);

        if (*s >= 0 && *t < 0 && *t - *s - c >= 0)
            set_bits(registers[SR], SR_V);
        else
            clr_bits(registers[SR], SR_V);
    }

    set_sr_flags(dval, bw);
}

void sub(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    sval = read_word(saddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval -= sval;
        write_word(daddr, dval);

        if (tmp - sval < 0)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);

        if (sval >= 0 && tmp < 0 && tmp - sval >= 0)
            set_bits(registers[SR], SR_V);
        else
            clr_bits(registers[SR], SR_V);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval -= sval;
        write_byte(daddr, (u8)dval);

        s8 *s = (s8 *)&sval;
        s8 *t = (s8 *)&tmp;

        if (tmp - sval < 0)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);

        if (*s >= 0 && *t < 0 && *t - *s >= 0)
            set_bits(registers[SR], SR_V);
        else
            clr_bits(registers[SR], SR_V);

    }
    set_sr_flags(dval, bw);
}

void cmp(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval -= sval;

        if (tmp - sval < 0)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);

        if (sval >= 0 && tmp < 0 && tmp - sval >= 0)
            set_bits(registers[SR], SR_V);
        else
            clr_bits(registers[SR], SR_V);
    } else {
        sval = read_byte(saddr);
        tmp = dval = read_byte(daddr);
        dval -= sval;

        s8 *s = (s8 *)&sval;
        s8 *t = (s8 *)&tmp;

        if (tmp - sval < 0)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);

        if (*s >= 0 && *t < 0 && *t - *s >= 0)
            set_bits(registers[SR], SR_V);
        else
            clr_bits(registers[SR], SR_V);
    }
    set_sr_flags(dval, bw);
}

/* XXX IMPLEMENT THIS CORRECTLY */
void dadd(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval += sval + read_bits(registers[SR], SR_C);
        write_word(daddr, dval);

        if (dval & 0x8000) {
            set_bits(registers[SR], SR_N);
            clr_bits(registers[SR], SR_Z);
        } else {
            clr_bits(registers[SR], SR_N);
            if (!dval)
                set_bits(registers[SR], SR_Z);
        }

        if (tmp + sval > 0xffff)
            set_bits(registers[SR], SR_V|SR_C);
        else
            clr_bits(registers[SR], SR_V|SR_C);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval += sval + read_bits(registers[SR], SR_C);
        write_byte(daddr, (u8)dval);

        if (dval & 0x80) {
            set_bits(registers[SR], SR_N);
            clr_bits(registers[SR], SR_Z);
        } else {
            clr_bits(registers[SR], SR_N);
            if (!dval)
                set_bits(registers[SR], SR_Z);
        }

        if (dval > 0xff)
            set_bits(registers[SR], SR_V|SR_C);
        else
            clr_bits(registers[SR], SR_V|SR_C);
    }
}

void bit(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg, wb;
    u16 sval, dval;
    u32 saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        dval = read_word(daddr);
        dval &= sval;
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval &= sval;
    }

    set_bits(registers[SR], (!!dval) << _SR_C);
    set_sr_flags(dval, bw);
    clr_bits(registers[SR], SR_V);
}

void bic(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        dval = read_word(daddr);
        dval &= ~sval;
        write_word(daddr, dval);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval &= (~sval & 0xff);
        write_byte(daddr, (u8)dval);
    }
}

void bis(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    sval = read_word(saddr);
    //printf("saddr: 0x%x (%x), daddr: 0x%x\n", saddr, sval, daddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        dval = read_word(daddr);
        dval |= sval;
        write_word(daddr, dval);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval |= sval;
        write_byte(daddr, (u8)dval);
    }
}

void xor(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        dval = read_word(daddr);
        dval ^= sval;
        write_word(daddr, dval);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval ^= sval;
        write_byte(daddr, (u8)dval);

        if (dval) {
            set_bits(registers[SR], SR_C);
            if (read_bits(sval, 0x80) && read_bits(dval, 0x80))
                set_bits(registers[SR], SR_V);
        } else
            clr_bits(registers[SR], SR_C);
    }
    set_sr_flags(dval, bw);
}

void and(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb, &sval);
    daddr = get_addr(ad, dreg, &wb, &dval);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        dval = read_word(daddr);
        dval &= sval;
        write_word(daddr, dval);


    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval &= sval;
        write_byte(daddr, (u8)dval);

    }
    set_sr_flags(dval, bw);
    clr_bits(registers[SR], SR_V);

    /* set C if result is NOT zero, reset otherwise */
    set_bits(registers[SR], (!!dval) << _SR_C);
}

static void (*call_table[])(u16) = {
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
    u16 opcode;

    opcode = read_bits(instruction, 0xf000) >> 12;

    call_table[opcode - 4](instruction);
}
