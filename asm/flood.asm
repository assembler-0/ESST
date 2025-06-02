section .text
global floodL1L2, floodMemory, rowhammerAttack, floodNt

floodL1L2:
    mov rcx, [rsi]          ; Load iterations count (second argument)
    mov rax, 0xdeadbeef
    lea r8, [rdi + rdx]     ; Calculate end pointer (rdx = buffer_size)
.cacheLoop:
    cmp rdi, r8
    jae .cacheDone
    mov [rdi], rax
    add rdi, 64
    dec rcx
    jnz .cacheLoop
.cacheDone:
    ret

rowhammerAttack:
    mov rcx, [rsi]          ; iterations count (second argument)
    mov r8, rdx             ; buffer_size (third argument)
    shr r8, 1               ; Split buffer in half
    lea r9, [rdi + r8]      ; Second hammer target
    mov rax, 0xAAAAAAAAAAAAAAA
.rhLoop:
    mov [rdi], rax
    clflush [rdi]
    mov [r9], rax
    clflush [r9]
    dec rcx
    jnz .rhLoop
    ret

; Add similar fixes to floodMemory and floodNt
floodMemory:
    mov rcx, [rsi]          ; iterations count
    mov rax, 0xbaadf00d
    lea r8, [rdi + rdx]     ; end pointer
.memoryLoop:
    cmp rdi, r8
    jae .memoryDone
    mov [rdi], rax
    mov r9, [rdi]           ; Use scratch register r9 instead of rdi
    add rdi, 128
    dec rcx
    jnz .memoryLoop
.memoryDone:
    ret

floodNt:
    mov rcx, [rsi]          ; iterations count
    mov rax, 0x1122334455667788
    lea r8, [rdi + rdx]     ; end pointer
.ntLoop:
    cmp rdi, r8
    jae .ntDone
    movnti [rdi], rax
    add rdi, 64
    dec rcx
    jnz .ntLoop
    sfence
.ntDone:
    ret