mov ah, 0x0e        ; BIOS tty mode

mov al, 'H'
int 0x10
mov al, 'e'
int 0x10
mov al, 'l'
int 0x10
int 0x10           ; prints second 'l'
mov al, 'o'
int 0x10

mov al, ' '        ; <-- SPACE
int 0x10

mov al, 'o'
int 0x10
mov al, 's'
int 0x10
mov al, 'u'
int 0x10

jmp $              ; infinite loop

; padding and boot signature
times 510 - ($-$$) db 0
dw 0xaa55

