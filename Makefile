all: msp430-emu

msp430-emu:
	$(MAKE) -C src
	@mv src/msp430-emu .

clean:
	$(MAKE) -C src clean
	-$(RM) msp430-emu
