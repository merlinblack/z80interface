# Find the great Z80 asembler zasm at https://github.com/Megatokio/zasm

all: mybios.rom ramimage.hex

mybios.rom:	mybios.asm lcd.asm
	zasm --opcodes --labels --cycles mybios.asm

mybios.hex:	mybios.asm lcd.asm
	zasm --opcodes --labels --cycles --hex --target=ram mybios.asm

mybios.inc: mybios.rom
	./genROMInc.py

burn: mybios.rom
	minipro --no_size_warning --device AT28C256 --write mybios.rom

ramimage.hex: ramimage.asm mybios.inc
	zasm --opcodes --labels --cycles --hex --target=ram ramimage.asm

flash: ramimage.hex
	./write.py ramimage.hex

flashbios: mybios.hex
	./write.py mybios.hex

clean:
	rm -f *.lst *.hex *.rom mybios.inc
