[BITS 32]

[EXTERN isr_handler_c]

[GLOBAL isr0]
[GLOBAL isr1]
[GLOBAL isr2]
[GLOBAL isr3]
[GLOBAL isr4]
[GLOBAL isr5]
[GLOBAL isr6]
[GLOBAL isr7]
[GLOBAL isr8]
[GLOBAL isr9]
[GLOBAL isr10]
[GLOBAL isr11]
[GLOBAL isr12]
[GLOBAL isr13]
[GLOBAL isr14]
[GLOBAL isr15]
[GLOBAL isr16]
[GLOBAL isr17]
[GLOBAL isr18]
[GLOBAL isr19]
[GLOBAL isr20]
[GLOBAL isr21]
[GLOBAL isr22]
[GLOBAL isr23]
[GLOBAL isr24]
[GLOBAL isr25]
[GLOBAL isr26]
[GLOBAL isr27]
[GLOBAL isr28]
[GLOBAL isr29]
[GLOBAL isr30]
[GLOBAL isr31]

[GLOBAL irq0]
[GLOBAL irq1]
[GLOBAL irq2]
[GLOBAL irq3]
[GLOBAL irq4]
[GLOBAL irq5]
[GLOBAL irq6]
[GLOBAL irq7]
[GLOBAL irq8]
[GLOBAL irq9]
[GLOBAL irq10]
[GLOBAL irq11]
[GLOBAL irq12]
[GLOBAL irq13]
[GLOBAL irq14]
[GLOBAL irq15]

[GLOBAL isr128]  ; Syscall interrupt
[GLOBAL tss_flush]

%macro ISR_NOERR 1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
isr%1:
    push dword %1
    jmp isr_common_stub
%endmacro

%macro IRQ 2
irq%1:
    push dword 0
    push dword %2
    jmp isr_common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; Syscall interrupt (int 0x80)
isr128:
    push dword 0
    push dword 128
    jmp isr_common_stub

; Load TSS into TR register
; TSS selector is at GDT index 5 = 0x28
tss_flush:
    mov ax, 0x28
    ltr ax
    ret

isr_common_stub:
    push ds
    push es
    push fs
    push gs
    pusha

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler_c
    add esp, 4

    popa
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 8
    iretd
