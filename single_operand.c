#include "common.h"

static void illegal_instr(u16 instruction)
{
    /* TODO not sure how to deal with illegal instructions yet */
}

static void rrc(u16 instruction)
{
    u16 addr, val;
    u8 reg, ad, tmpc;
    
    reg = read_bits(instruction, 0xf)
    ad = read_bits(instruction, 0x30) >> 4;

    switch (ad) {
        case 0:
            /* register */
            tmpc = registers[reg] & 0x1;
            registers[reg] = (registers[reg] >> 1) | (read_bits(registers[SR], SR_C) << 15);
            break;
        case 1:
            addr = inc_reg[PC];
            if (reg != SR) /* absolute mode */
                addr += registers[reg];
            val = read_word(addr);
            tmpc = val & 0x1;
            val = (val >> 1) | (read_bits(registers[SR], SR_C) << 15);
            write_word(addr, val);
            break;
        case 2:
            addr = registers[reg];
            val = read_word(addr);
            tmpc = val & 0x1;
            val = (val >> 1) | (read_bits(registers[SR], SR_C) << 15);
            write_word(addr, val);
            break;
        case 3:
            if (reg != PC) {
                addr = registers[reg];
                val = read_word(addr);
                tmpc = val & 0x1;
                val = (read_word(addr) >> 1) | (read_bits(registers[SR], SR_C) << 15);
                write_word(addr, val);
            }
            inc_reg(reg);
            break;
    }

    /* set bits in SR */
    registers[SR] = (registers[SR] & ~1) | tmpc;
    clr_bit(registers[SR], SR_V);

    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z)

    if (read_bits(val, 0x8000))
        set_bits(registers[SR], SR_N)
}

static void rrc_b(u16 instruction)
{
    u16 addr;
    u8 reg, ad, tmpc, val;
    
    reg = read_bits(instruction, 0xf)
    ad = read_bits(instruction, 0x30) >> 4;

    switch (ad) {
        case 0:
            /* register */
            val = registers[reg] & 0xff;
            tmpc = registers[reg] & 0x1;
            val = (registers[reg] >> 1) | (read_bits(registers[SR], SR_C) << 7);
            registers[reg] = (registers[reg] & 0xff00) | val;
            break;
        case 1:
            addr = inc_reg[PC];
            if (reg != SR) /* absolute mode */
                addr += registers[reg];
            val = read_byte(addr);
            tmpc = val & 0x1;
            val = (val >> 1) | (read_bits(registers[SR], SR_C) << 7);
            write_byte(addr, val);
            break;
        case 2:
            addr = registers[reg];
            val = read_byte(addr);
            tmpc = val & 0x1;
            val = (val >> 1) | (read_bits(registers[SR], SR_C) << 15);
            write_byte(addr, val);
            break;
        case 3:
            if (reg != PC) {
                addr = registers[reg];
                val = read_byte(addr);
                tmpc = val & 0x1;
                val = (val >> 1) | (read_bits(registers[SR], SR_C) << 7);
                write_byte(addr, val);
            }
            inc_reg(reg);
            break;
    }

    /* set bits in SR */
    registers[SR] = (registers[SR] & ~1) | tmpc;
    clr_bits(registers[SR], SR_V);

    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z)

    if (read_bits(val, 0x8000))
        set_bits(registers[SR], SR_N)
}

static void swpb(u16 instruction)
{
    u16 addr;
    u8 reg, ad, val;
    
    reg = read_bits(instruction, 0xf)
    ad = read_bits(instruction, 0x30) >> 4;

    switch (ad) {
        case 0:
            /* register */
            val = registers[reg];
            val = (val >> 8) | (val << 8);
            registers[reg] = val;
            break;
        case 1:
            addr = inc_reg[PC];
            if (reg != SR) /* absolute mode */
                addr += registers[reg];
            val = read_word(addr);
            val = (val >> 8) | (val << 8);
            write_word(addr, val);
            break;
        case 2:
            addr = registers[reg];
            val = read_word(addr);
            val = (val >> 8) | (val << 8);
            write_word(addr, val);
            break;
        case 3:
            if (reg != PC) {
                addr = registers[reg];
                val = read_word(addr);
                val = (val >> 8) | (val << 8);
                write_word(addr, val);
            }
            inc_reg(reg);
            break;
    }
}

static void rra(u16 instruction)
{
    u16 addr, msb, val;
    u8 reg, ad;
    
    reg = read_bits(instruction, 0xf)
    ad = read_bits(instruction, 0x30) >> 4;

    switch (ad) {
        case 0:
            /* register */
            val = registers[reg];
            msb = read_bits(val, 0x8000);
            registers[SR] = (registers[SR] & ~1) | (val & 1);
            val = (val >> 1) | msb;
            registers[reg] = val;
            break;
        case 1:
            addr = inc_reg[PC];
            if (reg != SR) /* absolute mode */
                addr += registers[reg];
            val = read_word(addr);
            msb = read_bits(val, 0x8000);
            registers[SR] = (registers[SR] & ~1) | (val & 1);
            val = (val >> 1) | msb;
            write_word(addr, val);
            break;
        case 2:
            addr = registers[reg];
            val = read_word(addr);
            msb = read_bits(val, 0x8000);
            registers[SR] = (registers[SR] & ~1) | (val & 1);
            val = (val >> 1) | msb;
            write_word(addr, val);
            break;
        case 3:
            if (reg != PC) {
                addr = registers[reg];
                val = read_word(addr);
                msb = read_bits(val, 0x8000);
                registers[SR] = (registers[SR] & ~1) | (val & 1);
                val = (val >> 1) | msb;
                write_word(addr, val);
            }
            inc_reg(reg);
            break;
    }

    clr_bit(registers[SR], SR_V);
    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z)

    if (read_bits(val, 0x8000))
        set_bits(registers[SR], SR_N)
}

static void rra_b(u16 instruction)
{
    u16 addr, msb;
    u8 reg, ad, val;
    
    reg = read_bits(instruction, 0xf)
    ad = read_bits(instruction, 0x30) >> 4;

    switch (ad) {
        case 0:
            /* register */
            val = registers[reg] & 0xff;
            msb = read_bits(val, 0x80);
            registers[SR] = (registers[SR] & ~1) | (val & 1);
            val = (val >> 1) | msb;
            registers[reg] = (registers[reg] & 0xff00) | val;
            break;
        case 1:
            addr = inc_reg[PC];
            if (reg != SR) /* absolute mode */
                addr += registers[reg];
            val = read_byte(addr);
            msb = read_bits(val, 0x80);
            registers[SR] = (registers[SR] & ~1) | (val & 1);
            val = (val >> 1) | msb;
            write_byte(addr, val);
            break;
        case 2:
            addr = registers[reg];
            val = read_word(addr);
            msb = read_bits(val, 0x80);
            registers[SR] = (registers[SR] & ~1) | (val & 1);
            val = (val >> 1) | msb;
            write_byte(addr, val);
            break;
        case 3:
            if (reg != PC) {
                addr = registers[reg];
                val = read_word(addr);
                msb = read_bits(val, 0x80);
                registers[SR] = (registers[SR] & ~1) | (val & 1);
                val = (val >> 1) | msb;
                write_byte(addr, val);
            }
            inc_reg(reg);
            break;
    }

    clr_bit(registers[SR], SR_V);
    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z)

    if (read_bits(val, 0x8000))
        set_bits(registers[SR], SR_N)
}

static void sxt(u16 instruction)
{

}

static void push(u16 instruction)
{

}

static void push_b(u16 instruction)
{

}

static void call(u16 instruction)
{

}

static void reti(u16 instruction)
{

}

static void (*call_table[])(u16) = {
    rrc,
    rrc_b,
    swpb,
    illegal_instr,
    rra,
    rra_b,
    sxt,
    illegal_instr,
    push,
    push_b,
    call,
    illegal_instr,
    reti,
};

void opc_components(u16 instruction, u16 *opcode, u8 &bw, u8 *ad, u8 dsreg)
{
    
}

void handle_single(u16 instruction)
{
    u16 opcode;

    opcode = read_bits(instruction, 0x0fc0) >> 6;

    if (opcode > 12) {
        /* TODO not sure how to deal with illegal instructions yet */
        illegal_instr(instruction);
        return;
    }

    call_table[type](instruction);

}
