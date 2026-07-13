section .text
global context_switch

; void context_switch(u64 *anciens_registres, u64 *nouveaux_registres, u64 nouveau_cr3);
; rdi = anciens_registres
; rsi = nouveaux_registres
; rdx = nouveau_cr3

context_switch:
    test rdi, rdi           
    jz .restauration        

    ; Sauvegarde de l'ancien contexte
    mov [rdi + 0x00], rbx
    mov [rdi + 0x08], rbp
    mov [rdi + 0x10], r12
    mov [rdi + 0x18], r13
    mov [rdi + 0x20], r14
    mov [rdi + 0x28], r15
    
    mov rax, [rsp]
    mov [rdi + 0x38], rax   
    
    lea rax, [rsp + 8]
    mov [rdi + 0x40], rax   

.restauration:
    ; Restauration du nouveau contexte
    mov rbx, [rsi + 0x00]
    mov rbp, [rsi + 0x08]
    mov r12, [rsi + 0x10]
    mov r13, [rsi + 0x18]
    mov r14, [rsi + 0x20]
    mov r15, [rsi + 0x28]
    
    mov rdi, [rsi + 0x30]
    mov rcx, [rsi + 0x38]   ; Nouveau RIP

    ; --- CHANGEMENT DE CONTEXTE MÉMOIRE SÉCURISÉ ---
    mov rsp, [rsi + 0x40]   ; 1. Basculer sur la NOUVELLE pile
    mov cr3, rdx            ; 2. Basculer sur le NOUVEAU PML4
    
    jmp rcx