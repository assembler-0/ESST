global p3np1E
section .text
; rdi = input
; rsi = steps pointer
p3np1E:
     cmp rdi, 1
     je .done
.loop:
     test rdi, 1
     jnz .odd
.even:
     shr rdi, 1
     inc qword [rsi]
     jmp .check
.odd:
     lea rdi, [rdi*2 + rdi + 1]
     inc qword [rsi]
.check:
     cmp rdi, 1
     jne .loop
.done:
     ret