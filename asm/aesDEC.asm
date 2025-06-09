section .text
global aes128DecryptBlock, aesXtsDecrypt

aes128DecryptBlock:
    vmovdqu xmm0, [rdx]             ; Load ciphertext (in = rdx)
    vpxor   xmm0, xmm0, [r8+160]    ; AddRoundKey (start with round 10 key)

    ; Rounds 9–1
    vaesdec xmm0, xmm0, [r8+144]
    vaesdec xmm0, xmm0, [r8+128]
    vaesdec xmm0, xmm0, [r8+112]
    vaesdec xmm0, xmm0, [r8+96]
    vaesdec xmm0, xmm0, [r8+80]
    vaesdec xmm0, xmm0, [r8+64]
    vaesdec xmm0, xmm0, [r8+48]
    vaesdec xmm0, xmm0, [r8+32]
    vaesdec xmm0, xmm0, [r8+16]

    vaesdeclast xmm0, xmm0, [r8]    ; Final round
    vmovdqu [rcx], xmm0             ; Store output
    ret

; extern "C" void aesXtsDecrypt(void* out, const void* in, const void* key, const void* tweak, size_t blocks);
aesXtsDecrypt:
    ; rcx = out, rdx = in, r8 = key, r9 = tweak
    ; stack arg [rsp+40] = blocks

    push    rbp
    mov     rbp, rsp
    and     rsp, -16                ; Align stack

    mov     rax, [rbp+40h]          ; Get 5th argument: blocks
    test    rax, rax
    jz      .done

    vmovdqu xmm15, [r9]            ; Load tweak

    mov     r10, rax
    shr     r10, 2                 ; 4-block chunks
    and     rax, 3                 ; Remaining

.process4blocks:
    test r10, r10
    jz .processRemaining

    ; Load 4 ciphertext blocks
    vmovdqu xmm11, [rsi]
    vmovdqu xmm12, [rsi+16]
    vmovdqu xmm13, [rsi+32]
    vmovdqu xmm14, [rsi+48]

    ; Apply initial tweak XOR
    vpxor xmm11, xmm11, xmm15
    vpxor xmm12, xmm12, xmm15
    vpxor xmm13, xmm13, xmm15
    vpxor xmm14, xmm14, xmm15

    ; Load round 14 key and apply (AES-256 decryption start)
    vmovdqu xmm0, [rdx+224]
    vpxor xmm11, xmm11, xmm0
    vpxor xmm12, xmm12, xmm0
    vpxor xmm13, xmm13, xmm0
    vpxor xmm14, xmm14, xmm0

    ; Rounds 13-1 for AES-256 (unrolled for maximum intensity)
    vmovdqu xmm0, [rdx+208]   ; Round 13
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+192]   ; Round 12
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+176]   ; Round 11
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+160]   ; Round 10
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+144]   ; Round 9
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+128]   ; Round 8
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+112]   ; Round 7
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+96]    ; Round 6
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+80]    ; Round 5
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+64]    ; Round 4
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+48]    ; Round 3
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+32]    ; Round 2
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+16]    ; Round 1
    vaesdec xmm11, xmm11, xmm0
    vaesdec xmm12, xmm12, xmm0
    vaesdec xmm13, xmm13, xmm0
    vaesdec xmm14, xmm14, xmm0

    ; Final round (round 0)
    vmovdqu xmm0, [rdx]
    vaesdeclast xmm11, xmm11, xmm0
    vaesdeclast xmm12, xmm12, xmm0
    vaesdeclast xmm13, xmm13, xmm0
    vaesdeclast xmm14, xmm14, xmm0

    ; Apply final tweak XOR
    vpxor xmm11, xmm11, xmm15
    vpxor xmm12, xmm12, xmm15
    vpxor xmm13, xmm13, xmm15
    vpxor xmm14, xmm14, xmm15

    ; Store results
    vmovdqu [rdi], xmm11
    vmovdqu [rdi+16], xmm12
    vmovdqu [rdi+32], xmm13
    vmovdqu [rdi+48], xmm14

    ; Update pointers for next 4 blocks
    add rdx, 64
    add rcx, 64
    dec r10
    jnz .process4blocks

.processRemaining:
    ; Handle remaining 1-3 blocks with intensive single-block processing
    test r8, r8
    jz .done

    ; Pre-load all round keys for maximum register utilization
    vmovdqu xmm0, [rdx+224]  ; Round 14
    vmovdqu xmm1, [rdx+208]  ; Round 13
    vmovdqu xmm2, [rdx+192]  ; Round 12
    vmovdqu xmm3, [rdx+176]  ; Round 11
    vmovdqu xmm4, [rdx+160]  ; Round 10
    vmovdqu xmm5, [rdx+144]  ; Round 9
    vmovdqu xmm6, [rdx+128]  ; Round 8
    vmovdqu xmm7, [rdx+112]  ; Round 7
    vmovdqu xmm8, [rdx+96]   ; Round 6
    vmovdqu xmm9, [rdx+80]   ; Round 5
    vmovdqu xmm10, [rdx+64]  ; Round 4

.remainingLoop:
    test    rax, rax
    jz      .done
    vmovdqu xmm11, [rsi]
    vpxor xmm11, xmm11, xmm15    ; Apply tweak

    ; Full AES-256 decryption with pre-loaded keys
    vpxor xmm11, xmm11, xmm0     ; Round 14
    vaesdec xmm11, xmm11, xmm1   ; Round 13
    vaesdec xmm11, xmm11, xmm2   ; Round 12
    vaesdec xmm11, xmm11, xmm3   ; Round 11
    vaesdec xmm11, xmm11, xmm4   ; Round 10
    vaesdec xmm11, xmm11, xmm5   ; Round 9
    vaesdec xmm11, xmm11, xmm6   ; Round 8
    vaesdec xmm11, xmm11, xmm7   ; Round 7
    vaesdec xmm11, xmm11, xmm8   ; Round 6
    vaesdec xmm11, xmm11, xmm9   ; Round 5
    vaesdec xmm11, xmm11, xmm10  ; Round 4

    ; Load remaining round keys on-demand
    vaesdec xmm11, xmm11, [rdx+48]   ; Round 3
    vaesdec xmm11, xmm11, [rdx+32]   ; Round 2
    vaesdec xmm11, xmm11, [rdx+16]   ; Round 1
    vaesdeclast xmm11, xmm11, [rdx]  ; Round 0

    vpxor xmm11, xmm11, xmm15    ; Apply final tweak
    vmovdqu [rdi], xmm11

    add rsi, 16
    add rdi, 16
    dec rax
    jnz .remainingLoop


.done:
    mov     rsp, rbp
    pop     rbp
    ret