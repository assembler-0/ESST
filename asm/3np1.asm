global p3np1E
section .text
; rdi = input
; rsi = out
p3np1E:
        cmp rdi, 1
        je .loopEosAt1
        jne .start
 .start:
        test rdi, 1
        jnz .isEven
        jz .isOdd
 .isOdd:
        imul rdi, 3
        add rdi, 1
        add rsi, 1
        jmp .checkLoopCondition
 .isEven:
        shr rdi, 1
        add rsi,1
        jmp .checkLoopCondition
 .checkLoopCondition:
        cmp rdi, 1
        jne .start
        je .loopEosAt1
 .loopEosAt1:
        ret