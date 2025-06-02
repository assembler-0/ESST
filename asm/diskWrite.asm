section .data
    align 4096
    ; Use multiple buffers with different patterns to stress cache
    buffer1 times 65536 db 0xAA
    buffer2 times 65536 db 0x55
    buffer3 times 65536 db 0xFF
    buffer4 times 65536 db 0x00

section .text
    global diskWrite

diskWrite:
    push rbp
    push r12
    push r13
    push r14
    push r15
    push rbx
    mov rbp, rsp

    mov r15, rdi                ; Save filename

    ; Create and write to file multiple times with different patterns
    mov rbx, 8                  ; Number of write cycles (8 * 2GB = 16GB total I/O)

.cycle_loop:
    ; sys_open - create new file each cycle for more I/O stress
    mov rax, 2
    mov rdi, r15
    mov rsi, 0x241              ; O_WRONLY | O_CREAT | O_TRUNC
    mov rdx, 0644o
    syscall

    cmp rax, 0
    jl .next_cycle
    mov r12, rax                ; Save fd

    ; Write 2GB with alternating buffer patterns
    mov r13, 8192               ; 8192 * 4 buffers * 64KB = 2GB per cycle

.write_loop:
    ; Write buffer1 (0xAA pattern)
    mov rax, 1
    mov rdi, r12
    lea rsi, [rel buffer1]
    mov rdx, 65536
    syscall
    cmp rax, 65536
    jne .close_file

    ; Write buffer2 (0x55 pattern)
    mov rax, 1
    mov rdi, r12
    lea rsi, [rel buffer2]
    mov rdx, 65536
    syscall
    cmp rax, 65536
    jne .close_file

    ; Write buffer3 (0xFF pattern)
    mov rax, 1
    mov rdi, r12
    lea rsi, [rel buffer3]
    mov rdx, 65536
    syscall
    cmp rax, 65536
    jne .close_file

    ; Write buffer4 (0x00 pattern)
    mov rax, 1
    mov rdi, r12
    lea rsi, [rel buffer4]
    mov rdx, 65536
    syscall
    cmp rax, 65536
    jne .close_file

    dec r13
    jnz .write_loop

.close_file:
    ; Close file
    mov rax, 3
    mov rdi, r12
    syscall

    ; Immediately delete file to keep disk usage low
    mov rax, 87                 ; sys_unlink
    mov rdi, r15
    syscall

.next_cycle:
    dec rbx
    jnz .cycle_loop

    ; Final cleanup
    mov rsp, rbp
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret