#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <msp430/common.h>
#include <elf_loader.h>

#include <debug_shell.h>

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
            //putchar(registers[15]);
            cons_printf("%c", (char)registers[15]);
            break;
        case 0x01:
            /* getchar.
             * Takes no arguments */
            //registers[15] = getchar();
            registers[15] = cons_getchar();
            break;
        case 0x02: /* terminate with EOF (ctrl + d) */
            /* gets.
             * Takes two arguments. The first is the address to place the
             * string, the second is the maximum number of bytes to read. Null
             * bytes are not handled specially null-terminated */
            //registers[15] = (u16)fread(memory + registers[15], 1, registers[14], stdin);
            registers[15] = (u16)cons_getsn(memory + registers[15], registers[14]);
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
            /* Interface with the deadbolt to trigger an unlock if the password
             * is correct.
             * Takes no arguments */
            cons_printf("Door unlocked!\n");
            break;
    }
}

static int exec_instruction(void)
{
    u16 curr_instr = read_word(registers[PC]);

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
            set_bits(registers[SR], SR_CPU_OFF);
            dec_reg(PC);
            return -1;
            break;
        default:
            /* double operand */
            handle_double(curr_instr);
    }

    if (read_bits(registers[SR], SR_CPU_OFF))
        return 0;

    if (registers[PC] == 0x10)
        handle_interrupt();

    return 1;
}

static int is_breakpoint(u16 addr, u32 *breakpoints, int size)
{
    int i;
    for (i = 0; i < size; i++)
        if (breakpoints[i] != -1 && addr == breakpoints[i])
            return 1;

    return 0;
}

int step(struct exec_info *info)
{
    int i, ret;

    for (i = 0; i < info->num_steps; i++) {
        ret = exec_instruction();
        if (ret == 0 || ret == -1)
            return ret;

        if (is_breakpoint(registers[PC], info->breakpoints, info->num_breakpoints))
            return 1;
    }
    return 2;
}

static void emulate(void)
{
    while (1) {
        if (!exec_instruction())
            break;
    }
}

static int load_file(FILE *fp)
{
    int ret, img_siz = 0;

    while (img_siz != sizeof(memory)) {
        ret = fread(memory + img_siz, 1, sizeof(memory) - img_siz, fp);

        if (!ret && feof(fp))
            break;
        else if (!ret) {
            perror("fopen");
            return 0;
        }

        img_siz += ret;
    }

    if (img_siz != sizeof(memory)) {
        fprintf(stderr, "The image size must be %ld bytes\n", sizeof(memory));
        return 0;
    }

    return 1;
}

static void init(int argc, char **argv)
{
    FILE *fp;
    char buffer[5];

    if ((fp = fopen(argv[1], "rb")) == NULL) {
        perror("fopen");
        exit(1);
    }

    fread(buffer, 4, 1, fp);
    buffer[4] = 0;

    memset(memory, 0, sizeof(memory));

    rewind(fp);
    /* check if file is an elf */
    if (!strncmp((const char *)buffer, "\177ELF", 4)) {
        if (!load_elf(fp)) {
            fprintf(stderr, "Failed to load elf file\n");
            exit(1);
        }
    } else {
        if (!load_file(fp)) {
            fprintf(stderr, "Failed to load file\n");
            exit(1);
        }
    }

    fclose(fp);

    srand(time(NULL));

    registers[PC] = read_word(0xfffe);
    write_word(0x10, 0x4130); // __trap_interrupt
}

extern int shell(void);

int main(int argc, char *argv[])
{
    init(argc, argv);

#ifdef DEBUG
    shell();
#else
    emulate();
#endif

    if (elf_is_loaded())
        clean_elf_stuff();

    return 0;
}
