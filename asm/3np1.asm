global p3np1E
section .text
p3np1E:
    xor rcx, rcx            ; RCX is now the first argument (a), so we need to adjust
    mov r8, rcx             ; Save original value (a) in R8
    mov r9, 3               ; Constant 3

    ; Windows x64 calling convention:
    ; First arg (a) in RCX
    ; Second arg (steps pointer) in RDX
    ; We need to use RDX instead of RSI for the steps pointer

.loop:
    lea rax, [r8 + 2*r8 + 1]
    shr r8, 1
    cmovc r8, rax
    inc rcx

    lea rax, [r8 + 2*r8 + 1]
    shr r8, 1
    cmovc r8, rax
    inc rcx

    lea rax, [r8 + 2*r8 + 1]
    shr r8, 1
    cmovc r8, rax
    inc rcx

    lea rax, [r8 + 2*r8 + 1]
    shr r8, 1
    cmovc r8, rax
    inc rcx

    cmp r8, 1
    ja .loop

    mov [rdx], rcx          ; Store result in steps pointer (now in RDX)
    ret