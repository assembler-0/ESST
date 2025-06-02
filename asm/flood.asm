section .text
global floodL1L2, floodMemory, rowhammerAttack, floodNt

; Intensive L1/L2 cache flooding with multiple access patterns
floodL1L2:
    mov rcx, [rsi]            ; iterations count (second argument)
    mov r8, rdi             ; save original buffer pointer
    lea r9, [rdi + rdx]     ; end pointer
    mov rax, 0xdeadbeefcafebabe
    mov r10, 0x1234567890abcdef
    mov r11, 0xfedcba0987654321

.cacheLoop:
    ; Multi-pattern cache flooding for maximum pressure
    mov r12, rdi            ; current position

    ; Pattern 1: Sequential write with prefetch
    cmp r12, r9
    jae .pattern2
    mov [r12], rax
    prefetchnta [r12 + 512]
    add r12, 64

.pattern2:
    ; Pattern 2: Stride-2 access
    cmp r12, r9
    jae .pattern3
    mov [r12], r10
    add r12, 128

.pattern3:
    ; Pattern 3: Reverse stride access
    mov r13, r9
    sub r13, 64
    cmp r13, rdi
    jb .pattern4
    mov [r13], r11

.pattern4:
    ; Pattern 4: Random-ish access pattern
    mov r13, rdi
    add r13, rcx
    and r13, rdx
    add r13, rdi
    cmp r13, r9
    jae .nextIter
    mov [r13], rax

.nextIter:
    add rdi, 192            ; Large stride to thrash cache
    cmp rdi, r9
    jb .cacheLoop

    ; Reset pointer and continue
    mov rdi, r8
    dec rcx
    jnz .cacheLoop

    ret

; Intensive memory flooding with multiple access patterns
floodMemory:
    mov rcx, [rsi]            ; iterations count
    mov r8, rdi             ; save original pointer
    lea r9, [rdi + rdx]     ; end pointer
    mov rax, 0xbaadf00dcafebabe
    mov r10, 0xdeadbeef12345678
    mov r11, 0xfeedface87654321

.memoryLoop:
    mov r12, rdi            ; current position

    ; Burst write pattern 1
.burst1:
    cmp r12, r9
    jae .burst2
    mov [r12], rax
    mov [r12 + 8], r10
    mov [r12 + 16], r11
    mov [r12 + 24], rax
    mov [r12 + 32], r10
    mov [r12 + 40], r11
    mov [r12 + 48], rax
    mov [r12 + 56], r10
    add r12, 256
    jmp .burst1

.burst2:
    ; Read-modify-write pattern
    mov r12, rdi
.rmw_loop:
    cmp r12, r9
    jae .burst3
    mov r13, [r12]
    xor r13, rax
    mov [r12], r13
    add r12, 128
    jmp .rmw_loop

.burst3:
    ; Non-temporal + regular stores mixed
    mov r12, rdi
.mixed_loop:
    cmp r12, r9
    jae .nextMemIter
    movnti [r12], rax
    mov [r12 + 64], r10
    movnti [r12 + 128], r11
    add r12, 192
    jmp .mixed_loop

.nextMemIter:
    sfence
    mov rdi, r8             ; reset pointer
    dec rcx
    jnz .memoryLoop

    ret

; Aggressive rowhammer with multiple targets and patterns
rowhammerAttack:
    mov rcx, [rsi]            ; iterations count
    mov r8, rdx             ; buffer_size
    shr r8, 2               ; Quarter buffer for multiple targets

    lea r9, [rdi + r8]      ; Target 1
    lea r10, [rdi + r8*2]   ; Target 2
    lea r11, [rdi + r8*2]   ; Target 3

    mov rax, 0xAAAAAAAAAAAAAAAA
    mov r12, 0x5555555555555555

.rhLoop:
    ; Multi-target hammering with different patterns

    ; Hammer sequence 1: Alternating pattern
    mov [rdi], rax
    mov [r9], r12
    clflush [rdi]
    clflush [r9]
    mfence

    ; Hammer sequence 2: Triple target
    mov [rdi], r12
    mov [r10], rax
    mov [r11], r12
    clflush [rdi]
    clflush [r10]
    clflush [r11]
    mfence

    ; Hammer sequence 3: Rapid fire
    mov [rdi], rax
    mov [rdi], r12
    mov [rdi], rax
    mov [rdi], r12
    clflush [rdi]

    mov [r9], r12
    mov [r9], rax
    mov [r9], r12
    mov [r9], rax
    clflush [r9]
    mfence

    ; Brief pause to avoid overwhelming memory controller
    pause
    pause

    dec rcx
    jnz .rhLoop

    ret

; Intensive non-temporal flooding with streaming patterns
floodNt:
    mov rcx, [rsi]            ; iterations count
    mov r8, rdi             ; save original pointer
    lea r9, [rdi + rdx]     ; end pointer

    mov rax, 0x1122334455667788
    mov r10, 0x99aabbccddeeff00
    mov r11, 0xffeeddccbbaa9988
    mov r12, 0x7766554433221100

.ntLoop:
    mov r13, rdi            ; current position

    ; Streaming pattern 1: Sequential burst
.stream1:
    cmp r13, r9
    jae .stream2
    movnti [r13], rax
    movnti [r13 + 8], r10
    movnti [r13 + 16], r11
    movnti [r13 + 24], r12
    movnti [r13 + 32], rax
    movnti [r13 + 40], r10
    movnti [r13 + 48], r11
    movnti [r13 + 56], r12
    add r13, 256
    jmp .stream1

.stream2:
    ; Streaming pattern 2: Interleaved with regular stores
    mov r13, rdi
.interleaved:
    cmp r13, r9
    jae .stream3
    movnti [r13], rax
    mov [r13 + 64], r10     ; Regular store to create pressure
    movnti [r13 + 128], r11
    mov [r13 + 192], r12    ; Regular store
    add r13, 320
    jmp .interleaved

.stream3:
    ; Streaming pattern 3: Reverse direction
    mov r13, r9
    sub r13, 64
.reverse:
    cmp r13, rdi
    jb .ntNext
    movnti [r13], rax
    sub r13, 128
    jmp .reverse

.ntNext:
    sfence                  ; Ensure all NT stores complete
    mov rdi, r8             ; reset pointer
    dec rcx
    jnz .ntLoop

    ret