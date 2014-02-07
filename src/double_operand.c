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

static u32 get_addr(u16 addr_mode, u16 reg, u16 *write_back)
{
    u32 addr = 0;
    *write_back = 1;

#ifdef DEBUG
    printf("addr_mode: %d, reg: %d\n", addr_mode, reg);
#endif

    switch (addr_mode) {
        case 0:
            /* register */
            addr = 0x10000 + (reg << 1);
            if (reg == CG)
                registers[CG] = 0;
            break;
        case 1:
            if (reg != CG) {
                addr = read_word(inc_reg(PC));
                addr += registers[reg];
            } else {
                registers[CG] = 1;
                addr = 0x10000 + (reg << 1);
            }
            break;
        case 2:
            if (reg == CG)
                registers[CG] = 2;
            else if (reg == SR)
                registers[CG] = 4;
            addr = 0x10000 + (CG << 1);
            break;
        case 3:
            if (reg != PC) {
                if (reg == CG)
                    registers[CG] = 0xffff;
                else if (reg == SR)
                    registers[CG] = 8;
                addr = 0x10000 + (registers[reg] << 1);
                inc_reg(reg);
            } else {
                addr = inc_reg(PC);
                *write_back = 0;
            }
            break;
    }

    return addr;
}

void mov(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval;
    u32 saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    //printf("saddr: 0x%x, daddr: 0x%x\n", saddr, daddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        write_word(daddr, sval);
    } else {
        sval = read_byte(saddr);
        write_byte(daddr, sval);
    }
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

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    sval = read_word(saddr);
    //printf("saddr: 0x%x (%x), daddr: 0x%x\n", saddr, sval, daddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval += sval;
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
        dval += sval;
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

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

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

void subc(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval -= sval - read_bits(registers[SR], SR_C);
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
        dval -= sval - read_bits(registers[SR], SR_C);
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

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    sval = read_word(saddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval -= sval;
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
        dval -= sval;
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

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval -= sval;

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
        dval -= sval;

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

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

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
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    //printf("saddr: 0x%x, daddr: 0x%x\n", saddr, daddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval &= sval;

        if (dval & 0x8000) {
            set_bits(registers[SR], SR_N);
            clr_bits(registers[SR], SR_Z);
        } else {
            clr_bits(registers[SR], SR_N);
            if (!dval)
                set_bits(registers[SR], SR_Z);
        }

        if (tmp + sval > 0xffff)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);
    } else {
        sval = read_byte(saddr);
        dval = read_byte(daddr);
        dval &= sval;

        if (dval & 0x80) {
            set_bits(registers[SR], SR_N);
            clr_bits(registers[SR], SR_Z);
        } else {
            clr_bits(registers[SR], SR_N);
            if (!dval)
                set_bits(registers[SR], SR_Z);
        }

        if (dval > 0xff)
            set_bits(registers[SR], SR_C);
        else
            clr_bits(registers[SR], SR_C);
    }
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
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval &= ~sval;
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
        dval &= (~sval & 0xff);
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

void bis(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    sval = read_word(saddr);
    //printf("saddr: 0x%x (%x), daddr: 0x%x\n", saddr, sval, daddr);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval |= sval;
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
        dval |= sval;
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

void xor(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval ^= sval;
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
        dval ^= sval;
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

void and(u16 instr)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    u16 opcode, sreg;
    u16 ad, bw, as, dreg;
    u16 wb, sval, dval;
    u32 tmp, saddr, daddr;

    opc_components(instr, &opcode, &sreg, &ad, &bw, &as, &dreg);

    saddr = get_addr(as, sreg, &wb);
    daddr = get_addr(ad, dreg, &wb);

    if (!wb)
        return;

    if (!bw) {
        sval = read_word(saddr);
        tmp = dval = read_word(daddr);
        dval &= sval;
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
        dval &= sval;
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
