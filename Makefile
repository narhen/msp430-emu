all: msp430-emu

msp430-emu:
	$(MAKE) -C lib elf_loader
	$(MAKE) -C src
	@mv src/msp430-emu .

debug:
	$(MAKE) EXTRA_CFLAGS="-D DEBUG" -C lib
	$(MAKE) EXTRA_CFLAGS="-D DEBUG" -C src debug
	@mv src/msp430-emu-debug .

debug2:
	$(MAKE) EXTRA_CFLAGS="-D DEBUG -D DEBUG2" -C lib
	$(MAKE) EXTRA_CFLAGS="-D DEBUG -D DEBUG2" -C src debug
	@mv src/msp430-emu-debug .

clean:
	$(MAKE) -C src clean
	$(MAKE) -C lib clean
	-$(RM) msp430-emu msp430-emu-debug
