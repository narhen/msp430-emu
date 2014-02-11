CC = gcc
CFLAGS = -Wall -I ../include/
LDFLAGS =

all: elf_loader debug_shell

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $< -c -o $@

elf_loader: elf_loader.o
	
debug_shell: debug_shell.o
	
clean:
	-$(RM) *.o
