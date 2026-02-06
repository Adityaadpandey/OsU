[BITS 16]
[ORG 0x7E00]

STAGE2_SECTORS      equ 16
KERNEL_LBA_START    equ 17
KERNEL_SECTORS      equ 128
KERNEL_LOAD_SEGMENT equ 0x1000
KERNEL_LOAD_OFFSET  equ 0x0000
SECTORS_PER_TRACK   equ 18
HEADS_PER_CYL       equ 2

GDT_CODE_SELECTOR   equ 0x08
GDT_DATA_SELECTOR   equ 0x10

magic:
    dw 0xBEEF

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    sti
    cld

    mov [boot_drive], dl

    mov si, msg_stage2
    call print_string

    call enable_a20
    call load_kernel

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp GDT_CODE_SELECTOR:protected_mode_entry

enable_a20:
    in al, 0x92
    or al, 0x02
    out 0x92, al
    ret

load_kernel:
    mov si, msg_kernel
    call print_string

    ; CHS fallback/read path for floppy-style images:
    ; start at LBA 17 => C=0, H=0, S=18
    mov ax, KERNEL_LOAD_SEGMENT
    mov es, ax
    mov bx, KERNEL_LOAD_OFFSET
    mov si, KERNEL_SECTORS
    mov ch, 0                  ; cylinder
    mov dh, 0                  ; head
    mov cl, 18                 ; sector (1-based)

.read_next:
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc .error

    add bx, 512

    dec si
    jz .ok

    inc cl
    cmp cl, SECTORS_PER_TRACK + 1
    jne .read_next

    mov cl, 1
    inc dh
    cmp dh, HEADS_PER_CYL
    jne .read_next

    mov dh, 0
    inc ch
    jmp .read_next

.ok:
    mov si, msg_done
    call print_string
    ret
.error:
    mov si, msg_error
    call print_string
    cli
    hlt
    jmp $

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

gdt_start:
    dq 0

    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

boot_drive: db 0

msg_stage2: db "OsU Stage2", 13, 10, 0
msg_kernel: db "Loading kernel...", 13, 10, 0
msg_done: db "done", 13, 10, 0
msg_error: db "kernel load failed", 13, 10, 0

[BITS 32]
protected_mode_entry:
    mov ax, GDT_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9FC00

    mov esi, 0x00010000
    mov edi, 0x00100000
    mov ecx, (KERNEL_SECTORS * 512) / 4
    rep movsd

    jmp 0x00100000

times STAGE2_SECTORS*512-($-$$) db 0
