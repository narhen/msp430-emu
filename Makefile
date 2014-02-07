all: msp430-emu

msp430-emu:
	$(MAKE) -C lib
	$(MAKE) -C src
	@mv src/msp430-emu .

debug:
	$(MAKE) EXTRA_CFLAGS="-D DEBUG" -C lib
	$(MAKE) EXTRA_CFLAGS="-D DEBUG" -C src
	@mv src/msp430-emu .

clean:
	$(MAKE) -C src clean
	$(MAKE) -C lib clean
	-$(RM) msp430-emu
