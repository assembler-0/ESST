; SHA-256 CPU Stress Test - NASM Syntax (PIC Compliant)
; Function: sha_stress(uint64_t iterations)
; Argument: RDI = number of iterations to run
; No return value - just pure CPU punishment
section .data
    align 64
stress_data:
    dq 0x0123456789abcdef, 0xfedcba9876543210
    dq 0x1111111111111111, 0x2222222222222222
    dq 0x3333333333333333, 0x4444444444444444
    dq 0x5555555555555555, 0x6666666666666666
    dq 0x7777777777777777, 0x8888888888888888
    dq 0x9999999999999999, 0xaaaaaaaaaaaaaaaa
    dq 0xbbbbbbbbbbbbbbbb, 0xcccccccccccccccc
    dq 0xdddddddddddddddd, 0xeeeeeeeeeeeeeeee
stress_data_end:

stress_data2:
    dq 0xdeadbeefcafebabe, 0xbabecafedeadbeef
    dq 0x0f0f0f0f0f0f0f0f, 0xf0f0f0f0f0f0f0f0
    dq 0x5a5a5a5a5a5a5a5a, 0xa5a5a5a5a5a5a5a5
    dq 0x1234567890abcdef, 0xfedcba0987654321
    dq 0x8765432109876543, 0x3456789012345678
    dq 0x9012345678901234, 0x4567890123456789
    dq 0x2109876543210987, 0x6543210987654321
    dq 0x0987654321098765, 0x4321098765432109
stress_data2_end:

    align 16
h0: dd 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a
h1: dd 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19

k_stress:
    dd 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5
    dd 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5
    dd 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3
    dd 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174

bswap_shuf:
    db 3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12

section .text
    global sha256
    global check_sha_support

sha256:
    ; Save registers
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 128

    ; Check iterations - if 0, default to 1 million
    test rdi, rdi
    jnz .start_stress
    mov rdi, 1000000

.start_stress:
    mov r15, rdi        ; r15 = iteration counter

    ; Load constants using RIP-relative addressing
    movdqa xmm14, [rel bswap_shuf]  ; Byte swap mask
    lea r12, [rel stress_data]      ; r12 = data pointer 1
    lea r13, [rel stress_data2]     ; r13 = data pointer 2
    lea r14, [rel k_stress]         ; r14 = K constants

.mega_loop:
    ; Initialize 4 hash states for parallel processing
    movdqu xmm0, [rel h0]      ; Hash state 1 (ABEF format)
    movdqu xmm1, [rel h1]      ; Hash state 1 (CDGH format)
    movdqu xmm2, [rel h0]      ; Hash state 2 (ABEF format)
    movdqu xmm3, [rel h1]      ; Hash state 2 (CDGH format)

    ; Rearrange for SHA-NI format
    pshufd xmm0, xmm0, 0xb1
    pshufd xmm1, xmm1, 0x1b
    movdqa xmm8, xmm0
    palignr xmm0, xmm1, 8
    pblendw xmm1, xmm8, 0xf0

    pshufd xmm2, xmm2, 0xb1
    pshufd xmm3, xmm3, 0x1b
    movdqa xmm9, xmm2
    palignr xmm2, xmm3, 8
    pblendw xmm3, xmm9, 0xf0

    ; Load data block end addresses for comparison
    lea rbx, [rel stress_data_end]
    lea rcx, [rel stress_data2_end] ; Use rcx as temp reg for 2nd addr

    ; Intensive round processing
    mov r8, 16         ; 16 rounds per block, use r8 for loop counter

.round_loop:
    ; Load and process data block 1
    movdqu xmm4, [r12]
    movdqu xmm5, [r12 + 16]
    pshufb xmm4, xmm14      ; Byte swap
    pshufb xmm5, xmm14      ; Byte swap

    ; SHA rounds on state 1
    movdqa xmm10, xmm0      ; Save for later
    movdqa xmm11, xmm1      ; Save for later
    sha256rnds2 xmm0, xmm1
    pshufd xmm4, xmm4, 0x4e
    sha256rnds2 xmm1, xmm0

    ; SHA rounds on state 2 with different data
    movdqa xmm12, xmm2      ; Save for later
    movdqa xmm13, xmm3      ; Save for later
    sha256rnds2 xmm2, xmm3
    pshufd xmm5, xmm5, 0x4e
    sha256rnds2 xmm3, xmm2

    ; Message schedule operations for maximum ALU usage
    movdqu xmm6, [r13]
    movdqu xmm7, [r13 + 16]
    pshufb xmm6, xmm14
    pshufb xmm7, xmm14
    sha256msg1 xmm6, xmm7   ; Message schedule
    sha256msg2 xmm6, xmm4   ; More message schedule

    ; More parallel operations
    paddd xmm0, xmm10       ; Add back initial state
    paddd xmm1, xmm11
    paddd xmm2, xmm12
    paddd xmm3, xmm13

    ; Rotate through different data
    add r12, 32 ; We used two 16-byte blocks from r12
    add r13, 32 ; We used two 16-byte blocks from r13
    cmp r12, rbx ; Compare with end address in rbx
    jl .no_wrap1
    lea r12, [rel stress_data]
.no_wrap1:
    cmp r13, rcx ; Compare with end address in rcx
    jl .no_wrap2
    lea r13, [rel stress_data2]
.no_wrap2:

    dec r8
    jnz .round_loop

    ; Extra CPU punishment - integer operations
    mov rax, r15
    mul rax             ; 64-bit multiply
    xor rbx, rax        ; XOR result
    ror rbx, 7          ; Rotate
    add rbx, r15        ; Add iteration counter

    ; Floating point stress
    cvtsi2sd xmm15, rbx
    sqrtsd xmm15, xmm15
    addsd xmm15, xmm15

    ; More SHA operations with computed values
    movq xmm4, rbx
    pshufd xmm4, xmm4, 0
    paddd xmm4, [r14] ; This is valid, r14 holds the address
    sha256rnds2 xmm0, xmm1

    ; Decrement and continue
    dec r15
    jnz .mega_loop

    ; Epilogue
    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; Function to check for SHA-NI support.
; Returns 1 in RAX if supported, 0 if not.
check_sha_support:
    push rbx
    push rcx
    push rdx

    ; Check for CPUID level 7 support
    mov eax, 0
    cpuid
    cmp eax, 7
    jl .no_sha_support

    ; CPUID.(EAX=7, ECX=0):EBX[29] is the SHA flag
    mov eax, 7
    mov ecx, 0
    cpuid

    ; Test bit 29 of EBX
    bt ebx, 29
    jc .has_sha_support  ; Jump if carry flag is set (bit was 1)

.no_sha_support:
    mov rax, 0
    jmp .done

.has_sha_support:
    mov rax, 1

.done:
    pop rdx
    pop rcx
    pop rbx
    ret