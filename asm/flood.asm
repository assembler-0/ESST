;
; flood_windows.asm
; Windows x64 NASM port of floodL1L2, floodMemory, rowhammerAttack, floodNt
; Calling convention: RCX, RDX, R8, R9 are first four args
; Callee-saved regs: RBX, RBP, RDI, RSI, R12-R15

section .text
    align 16
    global floodL1L2
    global floodMemory
    global rowhammerAttack
    global floodNt

; void floodL1L2(void* buffer (RCX), unsigned long* iterations_ptr (RDX), size_t buffer1_size (R8));
floodL1L2:
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rax, [rdx]            ; iterations count
    mov     r12, rcx              ; save orig buffer pointer
    lea     r13, [rcx + r8]       ; end pointer
    mov     r10, 0xDEADBEEFCAFEBABE
    mov     r11, 0x1234567890ABCDEF
    mov     r14, 0xFEDCBA0987654321

.cacheLoop:
    mov     r15, rcx              ; current position

    ; Pattern 1: Sequential write + prefetch
    cmp     r15, r13
    jae     .pattern2
    mov     [r15], r10
    prefetchnta [r15 + 512]
    add     r15, 64

.pattern2:
    ; Pattern 2: Stride-2 access
    cmp     r15, r13
    jae     .pattern3
    mov     [r15], r11
    add     r15, 128

.pattern3:
    ; Pattern 3: Reverse stride
    mov     rdi, r13
    sub     rdi, 64
    cmp     rdi, rcx
    jb      .pattern4
    mov     [rdi], r14

.pattern4:
    ; Pattern 4: Random-ish
    mov     rdi, rcx
    mov     rbx, rax
    and     rbx, r8
    add     rdi, rbx
    cmp     rdi, r13
    jae     .nextIter
    mov     [rdi], r10

.nextIter:
    add     rcx, 192
    cmp     rcx, r13
    jb      .cacheLoop

    ; Reset pointer and iterate
    mov     rcx, r12
    dec     rax
    jnz     .cacheLoop

    ; Restore and return
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret

; void floodMemory(void* buffer (RCX), unsigned long* iterations_ptr (RDX), size_t buffer_size (R8));
floodMemory:
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rax, [rdx]
    mov     r12, rcx              ; orig pointer
    lea     r13, [rcx + r8]
    mov     r10, 0xBAADF00DCAFEBABE
    mov     r11, 0xDEADBEEF12345678
    mov     r14, 0xFEEDFACE87654321

.memoryLoop:
    mov     r15, rcx

.burst1:
    cmp     r15, r13
    jae     .burst2
    mov     [r15],   r10
    mov     [r15+8], r11
    mov     [r15+16],r14
    mov     [r15+24],r10
    mov     [r15+32],r11
    mov     [r15+40],r14
    mov     [r15+48],r10
    mov     [r15+56],r11
    add     r15, 256
    jmp     .burst1

.burst2:
    mov     r15, rcx
.rmw_loop:
    cmp     r15, r13
    jae     .burst3
    mov     rbx, [r15]
    xor     rbx, r10
    mov     [r15], rbx
    add     r15, 128
    jmp     .rmw_loop

.burst3:
    mov     r15, rcx
.mixed_loop:
    cmp     r15, r13
    jae     .nextMemIter
    movnti  [r15],   r10
    mov     [r15+64],r11
    movnti  [r15+128],r14
    add     r15, 192
    jmp     .mixed_loop

.nextMemIter:
    sfence
    mov     rcx, r12
    dec     rax
    jnz     .memoryLoop

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret

; void rowhammerAttack(void* buffer (RCX), unsigned long* iterations_ptr (RDX), size_t buffer_size (R8));
rowhammerAttack:
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rax, [rdx]
    mov     r12, r8
    shr     r12, 2
    lea     r13, [rcx + r12]      ; target1
    lea     r14, [rcx + r12*2]    ; target2
    lea     r15, [rcx + r12*2]    ; target3
    mov     r10, 0xAAAAAAAAAAAAAAAA
    mov     r11, 0x5555555555555555

.rhLoop:
    ; Seq 1
    mov     [rcx],  r10
    mov     [r13],  r11
    clflush [rcx]
    clflush [r13]
    mfence

    ; Seq 2
    mov     [rcx],  r11
    mov     [r14],  r10
    mov     [r15],  r11
    clflush [rcx]
    clflush [r14]
    clflush [r15]
    mfence

    ; Seq 3
    mov     [rcx], r10
    mov     [rcx], r11
    mov     [rcx], r10
    mov     [rcx], r11
    clflush [rcx]
    mov     [r13], r11
    mov     [r13], r10
    mov     [r13], r11
    mov     [r13], r10
    clflush [r13]
    mfence

    pause
    pause
    dec     rax
    jnz     .rhLoop

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret

; void floodNt(void* buffer (RCX), unsigned long* iterations_ptr (RDX), size_t buffer_size (R8));
floodNt:
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rax, [rdx]
    mov     r12, rcx              ; orig ptr
    lea     r13, [rcx + r8]
    mov     r10, 0x1122334455667788
    mov     r11, 0x99AABBCCDDEEFF00
    mov     r14, 0xFFEEDDCCBBAA9988
    mov     r15, 0x7766554433221100

.ntLoop:
    mov     rbx, rcx

.stream1:
    cmp     rbx, r13
    jae     .stream2
    movnti  [rbx],    r10
    movnti  [rbx+8],  r11
    movnti  [rbx+16], r14
    movnti  [rbx+24], r15
    movnti  [rbx+32], r10
    movnti  [rbx+40], r11
    movnti  [rbx+48], r14
    movnti  [rbx+56], r15
    add     rbx, 256
    jmp     .stream1

.stream2:
    mov     rbx, rcx
.interleaved:
    cmp     rbx, r13
    jae     .stream3
    movnti  [rbx],   r10
    mov     [rbx+64],r11
    movnti  [rbx+128],r14
    mov     [rbx+192],r15
    add     rbx, 320
    jmp     .interleaved

.stream3:
    mov     rbx, r13
    sub     rbx, 64
.reverse:
    cmp     rbx, rcx
    jb      .ntNext
    movnti  [rbx], r10
    sub     rbx, 128
    jmp     .reverse

.ntNext:
    sfence
    mov     rcx, r12
    dec     rax
    jnz     .ntLoop

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret
