#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <msp430/common.h>
#include <elf_loader.h>

u16 prev_pc;

static void handle_interrupt(void)
{
    int type = read_bits(registers[SR], 0x7f00) >> 8;
//    u16 args = registers[SP] + 8;

    //printf("type: %x\n", type);
    switch (type) {
        case 0x00:
            /* putchar.
             * Takes one argument with the character to print */
            //putchar(read_word(args));
            putchar(registers[15]);
            break;
        case 0x01:
            /* getchar.
             * Takes no arguments */
            registers[15] = getchar();
            break;
        case 0x02: /* terminate with EOF (ctrl + d) */
            /* gets.
             * Takes two arguments. The first is the address to place the
             * string, the second is the maximum number of bytes to read. Null
             * bytes are not handled specially null-terminated */
            registers[15] = (u16)fread(memory + registers[15], 1, registers[14], stdin);
            break;
        case 0x10:
            /* turn on DEP.
             * Takes no arguments */
            break;
        case 0x11:
            /* mark a page as either only executable or only writable.
             * Takes two arguments. The first argument is the page number, the
             * second argument is 1 if writable, 0 if executable */
            break;
        case 0x20:
            /* rand.
             * Takes no arguments */
            registers[15] = (u16)rand();
            break;
        case 0x7d:
            /* Interface with the HSM-1. Set a flag in memory if the password in
             * is correct.
             * Takes two arguments: The first argument is the password to test,
             * the second is the location of a flag to overwrite if the password
             * is correct */
            break;
        case 0x7e:
            /* Interface with the HSM-2. Trigger the deadbolt unlock if the
             * password is correct.
             * Takes one argument: the password to test */
            break;
        case 0x7f:
            /* Interface with the deadbold to trigger an unlock if the password
             * is correct.
             * Takes no arguments */
            break;
    }
}

static void emulate(u16 main_addr)
{
    u16 curr_instr;

#ifdef DEBUG
    int flag = 0; /* did we reac main yet? */
#endif

    while (1) {
        curr_instr = read_word(registers[PC]);

#ifdef DEBUG
        if (!flag && registers[PC] == main_addr)
            flag = 1; /* indicates that we hit main() */

        putchar('\n');
        print_registers();
#endif

        prev_pc = registers[PC];
        inc_reg(PC);
        switch (read_bits(curr_instr, 0xf000)) {
            case 0x1000:
                /* single operand */
                handle_single(curr_instr);
                break;
            case 0x2000:
            case 0x3000:
                /* jumps */
                handle_jumps(curr_instr);
                break;
            case 0:
                printf("\nIllegal instruction @ 0x%x\n", registers[PC]);
                set_bits(registers[SR], SR_CPU_OFF);
                break;
            default:
                /* double operand */
                handle_double(curr_instr);
        }

        if (read_bits(registers[SR], SR_CPU_OFF))
            break;

        if (registers[PC] == 0x10)
            handle_interrupt();

#ifdef DEBUG
    /* break if we reached main */
    if (flag)
        getchar();
#endif
    }
}

/*
static void init(int argc, char **argv)
{
    int ret, img_siz = 0;
    FILE *fp;

    if (argc != 2) {
        fprintf(stderr, "USAGE: %s imgae\n", argv[0]);
        exit(1);
    }

    fp = fopen(argv[1], "r");

    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    while (img_siz != 0x10000) {
        ret = fread(memory + img_siz, 1, 0x10000 - img_siz, fp);

        if (!ret && feof(fp))
            break;
        else if (!ret) {
            perror("fopen");
            exit(1);
        }

        img_siz += ret;
    }

    if (img_siz != 0x10000) {
        fprintf(stderr, "The image size must be %d bytes\n", 0x10000);
        fclose(fp);
        exit(1);
    }

    fclose(fp);

    memset(registers, 0, sizeof(registers));
    registers[PC] = read_word(0xfffe);
}
*/

void segfault(int s)
{
    printf("\n-------\nSEGFAULT\n-------\n");
    printf("prev pc: %04x\n", prev_pc);
    set_bits(registers[SR], SR_CPU_OFF);
    print_registers();
    exit(1);
}

int main(int argc, char *argv[])
{
    u16 main_addr;

    signal(SIGSEGV, segfault);

    memset(memory, 0, sizeof(memory));
    if (!load_elf(argv[1], &main_addr)) {
        fprintf(stderr, "Failed to load elf file\n");
        return 1;
    }

    registers[PC] = read_word(0xfffe);
    write_word(0x10, 0x4130); // __trap_interrupt

    if (argc > 2)
        main_addr = (u16)strtoul(argv[2], NULL, 16); // break at this address instead

    srand(time(NULL));

    emulate(main_addr);

#ifdef DEBUG
    printf("\n\nCPUOFF flag set. Switching off CPU\n");
    print_registers();
#endif

    return 0;
}
