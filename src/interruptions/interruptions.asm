extern table_redirection

%macro CREER_ISR 1
global isr%1
isr%1:
    push rax          ; 1. SAUVEGARDER le RAX original tout de suite !
    mov rax, %1       ; 2. Mettre le numéro d'interruption dans RAX
    jmp rediriger_interruptions
%endmacro

; Génération des 256 ISR
%assign i 0
%rep 256
    CREER_ISR i
    %assign i i+1
%endrep

section .data
global table_adresses_isr
table_adresses_isr:
%assign i 0
%rep 256
    dq isr%+i
    %assign i i+1
%endrep

section .text
rediriger_interruptions:
    ; Sauvegarde de tous les registres
    push rcx; push rdx; push rbx; push rbp; push rsi; push rdi
    push r8; push r9; push r10; push r11; push r12; push r13; push r14; push r15

    mov rbx, [table_redirection + rax * 8]
    test rbx, rbx
    jz pas_de_gestionnaire
    mov rdi, rax   

    call rbx

pas_de_gestionnaire:
    ; Restauration des registres
    pop r15; pop r14; pop r13; pop r12; pop r11; pop r10; pop r9; pop r8
    pop rdi; pop rsi; pop rbp; pop rbx; pop rdx; pop rcx
    pop rax  
    iretq