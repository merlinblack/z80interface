	*include 'mybios.inc'

	org $8000
signature:
	db "Alara"
entry:
	jp start

message1:
	db "Nigel loves Alara!", 0
message2:
	db "Hello world from", 0
message3:
    db "a Z80!!!", 0
newline:
	db $0d, $0a, 0
buffer:
	ds 21, 0
itoa_buffer:
	ds 4, 0
iteration:
	db 0

start:
	ld a, 0
	ld (iteration), a

	call lcd_clear

	ld c, lcd_Line1
	ld hl, message1
	call lcd_write_on_line

	ld c, lcd_Line2
	ld hl, message2
	call lcd_write_on_line

	ld c, lcd_Line3
	ld hl, message3
	call lcd_write_on_line

main_loop:
#local

	ld a, (iteration)
	inc a
	ld (iteration), a

	ld hl, itoa_buffer
	ld a, (iteration)
	call itoa

	; clear buffer with spaces.
	ld hl, buffer
	ld b, 20
loop:
	ld (hl), 32	; space
	inc hl
	djnz loop

	ld hl, itoa_buffer
	; copy itoa_buffer to buffer right justified
	call strlen
	ld bc, hl
	xor a
	ld hl, buffer+20
	sbc hl, bc
	ld de, hl
	ld hl, itoa_buffer
	ldir

	ld c, lcd_Line4
	ld hl, buffer
	call lcd_write_on_line

	;ld a, 30
	;call delay

	jr main_loop
#endlocal


print_newline:
	ld hl, newline
	call serial_print
	ret

delay:
	ld b, 0
delay_loop:
	nop
	djnz delay_loop
	dec a
	jr nz, delay
	ret
