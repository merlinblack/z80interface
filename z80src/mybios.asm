
	org $0000

#local
RAMTOP equ $FFFF
RAMBOT equ $8000

boot:
	jp start

signature:
	db "Alara"
title_msg:
	db $0D, $0A, "Z80 picoBIOS copyleft Nigel Atkinson 2022", $0D, $0A, $0A, 0

start:
	ld sp, $0000	; SP will be top of RAM once it is pre-decremented
	ei
	ld hl, title_msg
	call print
	; Test for signature at $8000 and jump to $8005
	; if present. I.e. program has been loaded in RAM
	ld hl, RAMBOT
	ld de, signature
	ld bc, 5 ; length
signature_check:
	ld a, (de)
	inc de
	cpi
	jr nz, no_program_loaded
	jp pe, signature_check
	ld hl, program_msg
	call print
	jp $8005
no_program_loaded:
	ld hl, no_program_msg
	call print
	halt
	jp start

	org $66
handle_nmi:
	reti

no_program_msg:
	db "No program loaded. Halting.", $0D, $0A, 0
program_msg:
	db "Program signature detected. Jumping to entry point.", $0D, $0A, 0
#endlocal

	; null terminated string pointed to by HL
print:
#local
	ld a, (hl)
	and a
	ret z
	out ($70), a
	inc hl
	jr print
#endlocal

	; Four byte buffer pointed to by HL, uint8 in A
itoa:
#local
	push hl
	ld d, 0		; flag for hundreds place in use
	cp 200
	jr c, less_than_200
	ld (hl), 50	; 50 ascii = '2'
	sub 200
	inc hl
	inc d
	jr less_than_100
less_than_200:
	cp 100
	jr c, less_than_100
	ld (hl), 49 ; 49 ascii = '1'
	sub 100
	inc hl
	inc d
less_than_100:
	ld b, 0
divide_by_10:
	sub 10
	jr c, less_than_10
	inc b
	jr divide_by_10
less_than_10:
	add 10
	ld c, a		; remainder in c
	ld a, d
	and a
	ld a, b
	jr nz, tens_place
	and a
	jr z, ones_place
tens_place:
	add 48		; 48 = '0' in ascii
	ld (hl), a
	inc hl
ones_place:
	ld a, c
	add 48		; 48 = '0' in ascii
	ld (hl), a
	inc hl
	ld (hl), 0
	pop hl
	ret
#endlocal

; http://www.massmind.org/techref/zilog/z80/part4.htm
Div8:					; this routine performs the operation HL=HL/D
#local
	xor a				; clearing the upper 8 bits of AHL
	ld b,16				; the length of the dividend (16 bits)
Div8Loop:
	add hl,hl			; advancing a bit
	rla
	cp d				; checking if the divisor divides the digits chosen (in A)
	jp c,Div8NextBit	; if not, advancing without subtraction
	sub d				; subtracting the divisor
	inc l				; and setting the next digit of the quotient
Div8NextBit:
	djnz Div8Loop
	ret
#endlocal

