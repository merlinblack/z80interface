	; First program run by my Z80 breadboard computer
	; before it even had RAM.
    org $0000

start:
    ld hl, message
loop:
    ld a, (hl)
    and a
    jr z, start
    out ($AA), a
    inc hl
    jr loop

message:
    db "Nigel was here!", 0

