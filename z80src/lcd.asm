
lcd_Line1 equ	$00
lcd_Line2 equ	$40
lcd_Line3 equ	$14
lcd_Line4 equ	$54

lcd_Clear			equ 00000001b	; replace all characters with ASCII 'space'
lcd_Home            equ 00000010b	; return cursor to first position on first line
lcd_EntryMode       equ 00000110b	; shift cursor from left to right on read/write
lcd_DisplayOff      equ 00001000b	; turn display off
lcd_DisplayOn       equ 00001100b	; display on, cursor off, don't blink character
lcd_FunctionReset   equ 00110000b	; reset the LCD
lcd_FunctionSet8bit equ 00111000b	; 8-bit data, 2-line display, 5 x 7 font
lcd_SetCursor       equ 10000000b	; set cursor position

lcd_Instruction		equ $60			; IO Address for instrution register
lcd_Data			equ $61			; IO Address for data register

	; Wait until bit 7 from the lcd instruction register is clear
lcd_wait_if_busy:
#local
	ld b, $ff	; Wait at most FF iterations
loop:
	in a, (lcd_Instruction)
	rlca
	ret nc
	djnz loop
	ret
#endlocal

lcd_init:
	call lcd_wait_if_busy
	ld a, lcd_EntryMode
	out (lcd_Instruction), a

	call lcd_wait_if_busy
	ld a, lcd_DisplayOn
	out (lcd_Instruction), a

	call lcd_wait_if_busy
	ld a, lcd_FunctionSet8bit
	out (lcd_Instruction), a

	call lcd_wait_if_busy
	ld a, lcd_SetCursor
	out (lcd_Instruction), a
	ret

lcd_clear:
	call lcd_wait_if_busy
	ld a, lcd_Clear
	out (lcd_Instruction), a
	ret

lcd_write:
	call lcd_wait_if_busy
	ld a, (hl)
	and a
	ret z
	out (lcd_Data), a
	inc hl
	jr lcd_write

lcd_write_on_line:
	call lcd_set_cursor
	call lcd_write

lcd_set_cursor:
	call lcd_wait_if_busy
	ld a, lcd_SetCursor
	add a, c
	out (lcd_Instruction), a
	ret
