print equ $00BC
itoa equ $00C4

	org $8000
signature:
	db "Alara"
entry:
	jp start

message1:
	db "Nigel loves Alara <3 x x x", 0
message2:
	db "Hello world from a Z80!!! ", 0
message3:
	db "Iteration: ", 0
newline:
	db $0d, $0a, 0
buffer:
	db 0, 0, 0, 0
iteration:
	db 0

start:
	ld a, 0
	ld (iteration), a
	call print_newline
	call print_newline

main_loop:
	ld hl, message1
	call slow_print
	call print_newline

	ld hl, message2
	call slow_print
	call print_newline

	ld a, (iteration)
	inc a
	ld (iteration), a

	ld hl, message3
	call slow_print

	ld hl, buffer
	ld a, (iteration)
	call itoa
	call slow_print
	call print_newline
	call print_newline

	ld a, 30
	call delay

	jr main_loop

print_newline:
	ld hl, newline
	call print
	ret

	; null terminated string pointed to by HL
slow_print:
	ld a, (hl)
	and a
	ret z
	out ($AA), a
	ld a, 1
	call delay
	inc hl
	jr slow_print

delay:
	ld b, 0
delay_loop:
	nop
	djnz delay_loop
	dec a
	jr nz, delay
	ret
