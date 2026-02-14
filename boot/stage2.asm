[BITS 16]
[ORG 0x7E00]

STAGE2_SECTORS       equ 16
KERNEL_LBA_START     equ 17
KERNEL_SECTORS       equ 256
KERNEL_LOAD_SEGMENT  equ 0x1000
KERNEL_LOAD_OFFSET   equ 0x0000
KERNEL_CHUNK_SECTORS equ 64

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
    call setup_vbe

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

    mov word [remaining_sectors], KERNEL_SECTORS
    mov dword [current_lba], KERNEL_LBA_START
    mov word [dest_seg], KERNEL_LOAD_SEGMENT
    mov word [dest_off], KERNEL_LOAD_OFFSET

.read_loop:
    cmp word [remaining_sectors], 0
    je .ok

    mov ax, [remaining_sectors]
    cmp ax, KERNEL_CHUNK_SECTORS
    jbe .use_ax
    mov ax, KERNEL_CHUNK_SECTORS
.use_ax:
    mov [dap_count], ax
    mov ax, [dest_off]
    mov [dap_off], ax
    mov ax, [dest_seg]
    mov [dap_seg], ax
    mov eax, [current_lba]
    mov [dap_lba], eax
    mov dword [dap_lba+4], 0

    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc .error

    mov bx, [dap_count]
    sub [remaining_sectors], bx

    movzx eax, bx
    add [current_lba], eax

    mov cx, bx
    shl cx, 9
    mov ax, [dest_off]
    add ax, cx
    mov [dest_off], ax
    jnc .no_carry
    add word [dest_seg], 0x1000
.no_carry:
    jmp .read_loop

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

; VBE mode constants
VBE_MODE_800x600x32 equ 0x118   ; Preferred 32bpp mode (resolution may vary by BIOS)
VBE_MODE_INFO_ADDR  equ 0x8000  ; Where to store mode info for kernel

setup_vbe:
    ; Query VBE mode info for 800x600x32
    mov ax, 0x4F01              ; VBE Get Mode Info
    mov cx, VBE_MODE_800x600x32
    mov di, VBE_MODE_INFO_ADDR  ; Store at 0x8000
    int 0x10

    cmp ax, 0x004F              ; VBE success?
    jne .try_fallback

    ; Check if mode is usable (linear framebuffer supported)
    mov ax, [VBE_MODE_INFO_ADDR]  ; Mode attributes
    test ax, 0x80               ; Linear framebuffer bit
    jz .try_fallback

    ; Set the VBE mode!
    mov ax, 0x4F02              ; VBE Set Mode
    mov bx, VBE_MODE_800x600x32
    or bx, 0x4000               ; Use linear framebuffer
    int 0x10

    cmp ax, 0x004F
    jne .try_fallback

    mov word [vbe_mode_num], VBE_MODE_800x600x32
    ret

.try_fallback:
    ; Try mode 0x115 (800x600x24) as fallback
    mov ax, 0x4F01
    mov cx, 0x0115
    mov di, VBE_MODE_INFO_ADDR
    int 0x10

    cmp ax, 0x004F
    jne .no_vbe

    mov ax, 0x4F02
    mov bx, 0x0115
    or bx, 0x4000
    int 0x10

    cmp ax, 0x004F
    jne .no_vbe

    mov word [vbe_mode_num], 0x0115
    ret

.no_vbe:
    ; Clear mode info to signal no VBE - stay in text mode
    xor ax, ax
    mov di, VBE_MODE_INFO_ADDR
    mov cx, 256
    rep stosw
    mov word [vbe_mode_num], 0
    ret

vbe_mode_num: dw 0


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

dap:
    db 0x10
    db 0
dap_count:
    dw 0
dap_off:
    dw 0
dap_seg:
    dw 0
dap_lba:
    dq 0

remaining_sectors: dw 0
current_lba: dd 0
dest_seg: dw 0
dest_off: dw 0

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
