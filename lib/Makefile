CC = gcc
CFLAGS = -Wall -I ../include/
LDFLAGS =

all: elf_loader

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

elf_loader: elf_loader.o
	

clean:
	-$(RM) *.o
