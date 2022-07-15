# Find the great Z80 asembler zasm at https://github.com/Megatokio/zasm

mybios.rom:	mybios.asm
	zasm --opcodes --labels --cycles mybios.asm

burn: mybios.rom
	minipro --no_size_warning --device AT28C256 --write mybios.rom

ramimage.hex: ramimage.asm
	zasm --opcodes --labels --cycles --hex --target=ram ramimage.asm

upload: ramimage.hex
	./write.py

clean:
	rm -f *.lst *.hex *.rom
