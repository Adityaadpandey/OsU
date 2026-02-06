[BITS 32]

[GLOBAL _start]
[EXTERN kmain]
[EXTERN _bss_start]
[EXTERN _bss_end]

section .text
_start:
    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    call kmain

.hang:
    cli
    hlt
    jmp .hang
