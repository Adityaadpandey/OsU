; context.asm - Context switch implementation for x86
; Called as: context_switch(uint32_t **old_sp, uint32_t *new_sp)

section .text
global context_switch

; context_switch - Switch from one process to another
; Parameters:
;   [esp+4] = pointer to old process's stack pointer (to save current ESP)
;   [esp+8] = new process's stack pointer (to load)
;
; The stack of a suspended process looks like:
;   EDI <- top (lowest address)
;   ESI
;   EBP
;   EBX
;   EDX
;   ECX
;   EAX
;   EFLAGS
;   EIP <- return address (where to resume)
;   [parameters]

context_switch:
    ; Save the current process's registers
    pushfd              ; Save EFLAGS
    push eax
    push ecx
    push edx
    push ebx
    push ebp
    push esi
    push edi

    ; Save current stack pointer to *old_sp
    mov eax, [esp + 36]  ; Get old_sp (first parameter, +36 for pushes)
    test eax, eax        ; Check if old_sp is NULL
    jz .load_new         ; If NULL, skip saving
    mov [eax], esp       ; Save current ESP to *old_sp

.load_new:
    ; Load new stack pointer
    mov esp, [esp + 40]  ; Get new_sp (second parameter)
    ; Note: ESP is now pointing to new process's stack!

    ; Restore new process's registers
    pop edi
    pop esi
    pop ebp
    pop ebx
    pop edx
    pop ecx
    pop eax
    popfd               ; Restore EFLAGS

    ; Return to new process (EIP was pushed before context_switch was called)
    ret
