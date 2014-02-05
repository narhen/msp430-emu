CC = gcc
CFLAGS = -O3 -Wall -I ../include/
LDFLAGS =

all: msp430-emu

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

msp430-emu: main.o common.o jumps.o single_operand.o double_operand.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	-$(RM) *.o
