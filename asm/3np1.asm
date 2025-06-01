global p3np1E
section .text
p3np1E:
    xor rcx, rcx
    mov r8, rdi
    mov r9, 3

.loop:
    lea rax, [rdi + 2*rdi + 1]
    shr rdi, 1
    cmovc rdi, rax
    inc rcx

    lea rax, [rdi + 2*rdi + 1]
    shr rdi, 1
    cmovc rdi, rax
    inc rcx

    lea rax, [rdi + 2*rdi + 1]
    shr rdi, 1
    cmovc rdi, rax
    inc rcx

    lea rax, [rdi + 2*rdi + 1]
    shr rdi, 1
    cmovc rdi, rax
    inc rcx

    cmp rdi, 1
    ja .loop

    mov [rsi], rcx
    ret