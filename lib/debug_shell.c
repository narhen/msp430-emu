#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include <msp430/common.h>
#include <debug_shell.h>

struct window {
    WINDOW *win;
    int height, width;
    int curr; /* the current line inside the window */
};

u32 *breakpoints = NULL;
u32 *freelist = NULL;
int next_free = 0;
int num_breakpoints = 0;

int run = 1;

static struct window win_reg;
static struct window win_disas;
static struct window win_cons_out;
static struct window win_cons_in;
static struct window win_dbg_out;

static int insert_bpoint(bpoint)
{
    if (num_breakpoints >= 64)
        return 0;

    int ____tmp = freelist[next_free];
    breakpoints[next_free] = (bpoint);
    next_free = ____tmp;

    return 1;
}

static int del_bpoint(num)
{
    if (num < num_breakpoints && breakpoints[num] != -1) {
        breakpoints[num] = -1;
        freelist[num] = next_free;
        next_free = num;

        return 1;
    }

    return 0;
}

static void clear_window(WINDOW *win)
{
    wclear(win);
    wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
}

static WINDOW *create_newwin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0 , 0);		/* 0, 0 gives default characters 
                                 * for the vertical and horizontal
                                 * lines */
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);		/* Show that box 		*/

    return local_win;
}

static void print_win(struct window *win, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    vwprintw(win->win, fmt, ap);
    wrefresh(win->win);

    va_end(ap);
}

void cons_printf(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    vwprintw(win_cons_out.win, fmt, ap);
    wrefresh(win_cons_out.win);

    va_end(ap);
}

int cons_getchar(void)
{
    return wgetch(win_cons_out.win);
}

int cons_getsn(void *buf, int size)
{
    return wgetnstr(win_cons_out.win, buf, size);
}

static void shell_dump_memory(u16 addr, u16 size)
{
    int i, j;
    u8 *ptr, *tmpptr;

    tmpptr = memory + addr;
    for (i = 0; i < size; i += 16, tmpptr += 16) {
        print_win(&win_dbg_out, "%04lx:  ", tmpptr - memory);

        ptr = tmpptr;
        for (j = 0; j < 16; ++j, ++ptr) {
            print_win(&win_dbg_out, "%02x ", *ptr);
            if (j == 7)
                print_win(&win_dbg_out, " ");
        }

        print_win(&win_dbg_out, "    ");
        ptr = tmpptr;
        for (j = 0; j < 16; ++j, ++ptr)
            if (isprint(*ptr) && (!isspace(*ptr) || *ptr == ' '))
                print_win(&win_dbg_out, "%c", *ptr);
            else
                print_win(&win_dbg_out, "%c", '.');
        print_win(&win_dbg_out, "\n");
    }
}

static void update_regs(void)
{
    char buffer[256];
    char tmpbuf[128];

    clear_window(win_reg.win);

    win_reg.curr = 0;

    sprintf(buffer, " pc: %04x  sp: %04x  sr: %04x  cg: %04x\n", registers[PC], registers[SP],
            registers[SR], registers[CG]);
    print_win(&win_reg, buffer);

    sprintf(buffer, "r04: %04x r05: %04x ", registers[4], registers[5]);
    sprintf(tmpbuf, "r06: %04x r07: %04x\n", registers[6], registers[7]);
    strcat(buffer, tmpbuf);
    print_win(&win_reg, buffer);

    sprintf(buffer, "r08: %04x r09: %04x ", registers[8], registers[9]);
    sprintf(tmpbuf, "r10: %04x r11: %04x\n", registers[10], registers[11]);
    strcat(buffer, tmpbuf);
    print_win(&win_reg, buffer);

    sprintf(buffer, "r12: %04x r13: %04x ", registers[12], registers[13]);
    sprintf(tmpbuf, "r14: %04x r15: %04x\n", registers[14], registers[15]);
    strcat(buffer, tmpbuf);
    print_win(&win_reg, buffer);
}

static void print_disas(void)
{
    FILE *fp;
    char buf[256];

    if ((fp = fopen("../dummy.disas", "r")) == NULL) {
        print_win(&win_disas, "Failed to load disas file\n");
        return;
    }

    memset(buf, 0, sizeof(buf));
    while (fread(buf, 1, sizeof(buf) - 1, fp)) {
        print_win(&win_disas, buf);
        memset(buf, 0, sizeof(buf));
    }

    fclose(fp);
}

static void init(void)
{
    int i;

    initscr();
    refresh();

    win_cons_out.height = (int)((LINES / 100.0) * 70.0);
    win_cons_out.width = (int)((COLS / 100.0) * 30.0);
    win_cons_out.win = create_newwin(win_cons_out.height, win_cons_out.width,
            (int)((LINES / 100.0) * 30.0), (int)((COLS / 100.0) * 70.0));

    win_disas.height = (int)((LINES / 100.0) * 50.0);
    win_disas.width = (int)((COLS / 100.0) * 70.0);
    win_disas.win = create_newwin(win_disas.height, win_disas.width,
            0, 0);

    win_dbg_out.height = (int)((LINES / 100.0) * 40.0);
    win_dbg_out.width = (int)((COLS / 100.0) * 70.0);
    win_dbg_out.win = create_newwin(win_dbg_out.height, win_dbg_out.width,
            (int)((LINES / 100.0) * 50.0), 0);

    win_cons_in.height = (int)((LINES / 100.0) * 10.0);
    win_cons_in.width = (int)((COLS / 100.0) * 70.0);
    win_cons_in.win = create_newwin(win_cons_in.height, win_cons_in.width,
            (int)((LINES / 100.0) * 90.0), 0);

    win_reg.height = (int)((LINES / 100.0) * 30.0);
    win_reg.width = (int)((COLS / 100.0) * 30.0);
    win_reg.win = create_newwin(win_reg.height, win_reg.width,
            0, (int)((COLS / 100.0) * 70.0));

    scrollok(win_cons_in.win, 1);
    idlok(win_cons_in.win, 1);

    scrollok(win_cons_out.win, 1);
    idlok(win_cons_out.win, 1);

    scrollok(win_dbg_out.win, 1);
    idlok(win_dbg_out.win, 1);

    breakpoints = malloc(64 * sizeof(u32 *));
    freelist = malloc(64 * sizeof(u32 *));

    for (i = 0; i < 64; i++)
        freelist[i] = i + 1;
    freelist[63] = -1;
    memset(breakpoints, -1, sizeof(breakpoints));

    print_win(&win_disas, "Disassembly window\n");
}

extern int step(struct exec_info *info);
static int parse_command(char *str)
{
    int i;
    char buffer[128];
    char *args[10];
    char *saveptr1, *tmp = " ", *token;
    struct exec_info info = {breakpoints, num_breakpoints, 1};
    u32 tmp2;

    token = args[0] = strtok_r(str, tmp, &saveptr1);
    for (i = 1; i < 10 && token; ++i)
        token = args[i] = strtok_r(NULL, tmp, &saveptr1);

    if (args[0] == NULL)
        return 0;

    if (!strcmp(args[0], "c") || !strcmp(args[0], "continue")) {
        info.num_steps = 1;
        while ((i = step(&info)) == 2);
        if (i == -1) {
            print_win(&win_dbg_out, "\n\nIllegal instruction!\n");
            return 0;
        } else if (!i) {
            print_win(&win_cons_out, "\n\nCPUOFF flag set.\n");
            return 0;
        }
        return i;
    } else if (!strcmp(args[0], "b") || !strcmp(args[0], "break")) {
        if (args[1] == NULL) {
            print_win(&win_dbg_out, "break requires a second argument!\n");
        } else {
            if (args[1][0] == '0' && (args[1][1] == 'x' || args[1][1] == 'X'))
                tmp2 = strtoul(args[1], NULL, 16);
            else
                tmp2 = strtoul(args[1], NULL, 10);
            if (!insert_bpoint(tmp2))
                print_win(&win_dbg_out, "You can't have more than 64 breakpoints!\n");
            ++num_breakpoints;
            sprintf(buffer, "Created new breakpoint @ 0x%04x\n", tmp2);
            print_win(&win_dbg_out, buffer);

            return 1;
        }
    } else if (!strcmp(args[0], "i") || !strcmp(args[0], "info")) {
        if (!strcmp(args[1], "b") || !strcmp(args[1], "breakpoints")) {
            for (i = 0; i < num_breakpoints; i++) {
                if (breakpoints[i] != -1) {
                    sprintf(buffer, "%d: 0x%04x\n", i, breakpoints[i]);
                    print_win(&win_dbg_out, buffer);
                }
            }
        }
        return 1;
    } else if (!strcmp(args[0], "d") || !strcmp(args[0], "del")) {
        if (args[1][0] != 'a') {
            tmp2 = strtoul(args[1], NULL, 10);
            if (tmp2 < num_breakpoints)
                del_bpoint(tmp2);
        } else if (args[1][0] == 'a' || !strcmp(args[1], "all")) {
            memset(breakpoints, -1, sizeof(breakpoints));
            for (i = 0; i < 64; i++)
                freelist[i] = i + 1;
            freelist[63] = -1;
            next_free = 0;
        }
        return 1;
    } else if (!strcmp(args[0], "s") || !strcmp(args[0], "step")) {
        if (args[1] != NULL) {
            tmp2 = strtoul(args[1], NULL, 10);
            if (tmp2 == 0)
                tmp2 = 1;
            info.num_steps = tmp2;
        }
        i = step(&info);
        if (i == -1) {
            print_win(&win_cons_out, "\n\nIllegal instruction!\n");
            return 0;
        } else if (!i) {
            print_win(&win_cons_out, "\n\nCPUOFF flag set.\n");
            return 0;
        }
        return i;
    } else if (!strcmp(args[0], "x") || !strcmp(args[0], "examine")) {
        u16 num_bytes;
        u16 addr;
        if (args[1] == NULL) {
            print_win(&win_dbg_out, "Second parameter must be the number of bytes to examine\n");
            return 0;
        }

        if (args[2] == NULL) {
            print_win(&win_dbg_out, "Third parameter must be the address to read\n");
            return 0;
        }
        num_bytes = atoi(args[1]);
        if (args[2][0] == '0' && (args[2][1] == 'x' || args[2][1] == 'X'))
            addr = strtoul(args[2], NULL, 16);
        else
            addr = strtoul(args[2], NULL, 10);
        shell_dump_memory(addr, num_bytes);
        return 0;
    } else if (!strcmp(args[0], "quit")) {
        run = 0;
        return 0;
    }

    print_win(&win_dbg_out, "No such command!\n");
    return 1;
}

int shell(void)
{
    char buffer[256];

    init();
    raw();


    while (run) {
        update_regs();
        print_win(&win_cons_in, "> ");
        wgetstr(win_cons_in.win, buffer);

        parse_command(buffer);
    }


    endwin();
    free(breakpoints);

    return 0;
}
