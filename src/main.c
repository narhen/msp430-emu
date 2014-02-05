#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <msp430/common.h>
#include <elf_loader.h>

static void emulate(void)
{
    u16 curr_instr;

    while (1) {
        curr_instr = read_word(registers[PC]);

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
            default:
                /* double operand */
                handle_double(curr_instr);
        }
    }
}

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

int main(int argc, char *argv[])
{
    /*
    init(argc, argv);
    */

    if (!load_elf(argv[1])) {
        fprintf(stderr, "Failed to load elf file\n");
        return 1;
    }

    memset(registers, 0, sizeof(registers));
    registers[PC] = read_word(0xfffe);

    //emulate();

    print_registers();

    return 0;
}
