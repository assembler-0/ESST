global p3np1E
section .text
p3np1E:
        xor rsi, rsi
        cmp rdi, 1
        je .loop_eos_at_1
.loop_eos_at_1
        ret