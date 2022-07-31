
	org $0000

#local
RAMTOP equ $FFFF
RAMBOT equ $8000

boot:
	jp start

signature:
	db "Alara"
title_msg:
	db "Z80 picoBIOS (C)", 0
title_msg2:
	db "Nigel Atkinson 2022", 0

	org $66
handle_nmi:
	reti

start:
	ld sp, $0000	; SP will be top of RAM once it is pre-decremented
	ei

	call lcd_init
	call lcd_clear

	ld hl, title_msg
	ld c, lcd_Line1
	call lcd_write_on_line

	ld hl, title_msg2
	ld c, lcd_Line2
	call lcd_write_on_line

	call signature_check
	jr nz, no_program_loaded

	ld hl, program_msg
	ld c, lcd_Line3
	call lcd_write_on_line

	ld hl, program_msg2
	ld c, lcd_Line4
	call lcd_write_on_line

	jp $8005

no_program_loaded:
	ld hl, no_program_msg
	ld c, lcd_Line3
	call lcd_write_on_line

	ld hl, no_program_msg2
	ld c, lcd_Line4
	call lcd_write_on_line

	halt
	jp start

signature_check:
	; Test for signature at $8000 and jump to $8005
	; if present. I.e. program has been loaded in RAM
	ld hl, RAMBOT
	ld de, signature
	ld bc, 5 ; length
signature_check_loop:
	ld a, (de)
	inc de
	cpi
	ret nz ;no_program_loaded
	jp pe, signature_check_loop
	ret ; found signature (z will be set)

no_program_msg:
	;   12345678901234567890
	;                       12345678901234567890
	db "No program loaded", 0
no_program_msg2:
	db "Halting.", 0
program_msg:
	db "Program sig detected", 0
program_msg2:
	db "Jumping to entry.", 0
#endlocal

	; null terminated string pointed to by HL
serial_print:
#local
	ld a, (hl)
	and a
	ret z
	out ($70), a
	inc hl
	jr serial_print
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

	; get the length of a c style string at hl
	; return result in hl
strlen:
	ld de, hl
	ld a, 0
	ld bc, 0
	cpir
	dec hl	; don't count the terminating zero
	add a	; reset carry
	sbc hl, de
	ret

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

#include "lcd.asm"
