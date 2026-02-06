[BITS 16]
[ORG 0x7C00]

STAGE2_LOAD_SEGMENT equ 0x0000
STAGE2_LOAD_OFFSET  equ 0x7E00
STAGE2_SECTORS      equ 16

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    cld

    mov [boot_drive], dl

    mov si, msg_boot
    call print_string

    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [boot_drive]
    mov bx, STAGE2_LOAD_OFFSET
    int 0x13
    jc disk_error

    cmp word [STAGE2_LOAD_OFFSET], 0xBEEF
    jne stage2_error

    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET+2

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp print_string
.done:
    ret

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp hang

stage2_error:
    mov si, msg_stage2_error
    call print_string
    jmp hang

hang:
    cli
    hlt
    jmp hang

boot_drive: db 0
msg_boot: db "OsU MBR", 13, 10, 0
msg_disk_error: db "Disk read error", 13, 10, 0
msg_stage2_error: db "Stage2 invalid", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
