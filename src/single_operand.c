#include <stdio.h>

#include <msp430/common.h>

static void illegal_instr(u16 instruction)
{
    /* TODO not sure how to deal with illegal instructions yet */
}

static u32 get_addr(u8 instr, u16 *write_back)
{
    u32 addr = 0;
    u8 reg = read_bits(instr, 0xf);
    u8 addr_mode = read_bits(0x30, instr) >> 4;
    *write_back = 1;

    switch (addr_mode) {
        case 0:
            addr = 0x10000 + (reg << 1);
            break;
        case 1:
            addr = inc_reg(PC);
            if (reg != SR) /* absolute mode */
                addr += registers[reg];
            break;
        case 2:
            addr = registers[reg];
            break;
        case 3:
            if (reg != PC)
                addr = registers[reg];
            else {
                addr = inc_reg(PC);
                *write_back = 0;
            }
            inc_reg(reg);
            break;
    }

    return addr;
}

static void rrc(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 val, wb;
    u8 tmpc;

    addr = get_addr(instruction, &wb);
    val = read_word(addr);

    tmpc = val & 1;
    val = (val >> 1) | (read_bits(registers[SR], SR_C) << 15);

    if (wb)
        write_word(addr, val);

    /* set bits in SR */
    registers[SR] = (registers[SR] & ~1) | tmpc;
    clr_bits(registers[SR], SR_V);

    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z);

    if (read_bits(val, 0x8000))
        set_bits(registers[SR], SR_N);
}

static void rrc_b(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u8 tmpc, val;
    u16 wb;

    addr = get_addr(instruction, &wb);
    val = read_byte(addr);

    tmpc = val & 1;
    val = (val >> 1) | (read_bits(registers[SR], SR_C) << 7);

    if (wb)
        write_byte(addr, val);

    /* set bits in SR */
    registers[SR] = (registers[SR] & ~1) | tmpc;
    clr_bits(registers[SR], SR_V);

    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z);

    if (read_bits(val, 0x80))
        set_bits(registers[SR], SR_N);
}

static void swpb(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 val, wb;

    addr = get_addr(instruction, &wb);

    val = read_word(addr);
    val = (val >> 8) | (val << 8);    

    if (wb)
        write_word(addr, val);
}

static void rra(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 msb, val, wb;
    
    addr = get_addr(instruction, &wb);
    val = read_word(addr);
    msb = read_bits(val, 0x8000);

    val = (val >> 1) | msb;

    if (wb)
        write_word(addr, val);

    clr_bits(registers[SR], SR_V);
    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z);

    if (read_bits(val, 0x8000))
        set_bits(registers[SR], SR_N);
}

static void rra_b(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 wb;
    u8 val, msb;
    
    addr = get_addr(instruction, &wb);
    val = read_byte(addr);
    msb = read_bits(val, 0x80);

    val = (val >> 1) | msb;

    if (wb)
        write_word(addr, val);

    clr_bits(registers[SR], SR_V);
    if (val)
        clr_bits(registers[SR], SR_Z|SR_N);
    else
        set_bits(registers[SR], SR_Z);

    if (read_bits(val, 0x80))
        set_bits(registers[SR], SR_N);
}

static void sxt(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 wb;
    u8 val;
    
    addr = get_addr(instruction, &wb);
    val = read_byte(addr);

    if (read_bits(val, 0x80)) {
        val |= 0xff;
        set_bits(registers[SR], SR_N); /* set Negative bit if negative */
        if (wb)
            write_word(addr, val);
    }

    clr_bits(registers[SR], SR_V);
    if (val) {
        clr_bits(registers[SR], SR_Z);
        set_bits(registers[SR], SR_C);
        if (!read_bits(val, 0x8000))
            clr_bits(registers[SR], SR_N);
    } else {
        set_bits(registers[SR], SR_Z);
        clr_bits(registers[SR], SR_C);
    }
}

static void push(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 val, wb;
    
    addr = get_addr(instruction, &wb);
    val = read_word(addr);

    write_word(dec_reg(registers[SP]), val);
}

static void push_b(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 val, wb;
    
    addr = get_addr(instruction, &wb);
    val = (u16)read_byte(addr);

    write_word(dec_reg(registers[SP]), val);
}

static void call(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    u32 addr;
    u16 val, wb;
    
    addr = get_addr(instruction, &wb);
    val = read_word(addr);

    write_word(dec_reg(registers[SP]), registers[PC]);
    registers[PC] = val;
}

static void reti(u16 instruction)
{
#ifdef DEBUG
    printf("%s\n", __FUNCTION__);
#endif
    printf("%s\n", __FUNCTION__);
    registers[SR] = read_word(registers[SP]);
    inc_reg(registers[SP]);
    registers[PC] = read_word(registers[SP]);
    inc_reg(registers[SP]);
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

static inline void opc_components(u16 instruction, u16 *opcode, u8 *bw, u8 *ad, u8 *dsreg)
{
    *opcode = read_bits(instruction, 0x0f80) >> 7;
    *bw = read_bits(instruction, 0x40) >> 6;
    *ad = read_bits(instruction, 0x30) >> 4;
    *dsreg = read_bits(instruction, 0xf);
}

void handle_single(u16 instruction)
{
    u16 opcode, type;
    u8 bw, ad, dsreg;

    opc_components(instruction, &opcode, &bw, &ad, &dsreg);

    type = (opcode << 1) | bw;
    if (type > 12) {
        /* TODO not sure how to deal with illegal instructions yet */
        illegal_instr(instruction);
        return;
    }

    call_table[type](instruction);
}
