section .text
global aes128EncryptBlock, aes256Keygen, aesXtsEncrypt


aes128EncryptBlock:
    vmovdqu xmm0, [rsi]             ; Load plaintext
    vpxor xmm0, xmm0, [rdx]         ; AddRoundKey (round 0)

    ; Rounds 1-9 with proper round keys
    vaesenc xmm0, xmm0, [rdx+16]    ; Round 1
    vaesenc xmm0, xmm0, [rdx+32]    ; Round 2
    vaesenc xmm0, xmm0, [rdx+48]    ; Round 3
    vaesenc xmm0, xmm0, [rdx+64]    ; Round 4
    vaesenc xmm0, xmm0, [rdx+80]    ; Round 5
    vaesenc xmm0, xmm0, [rdx+96]    ; Round 6
    vaesenc xmm0, xmm0, [rdx+112]   ; Round 7
    vaesenc xmm0, xmm0, [rdx+128]   ; Round 8
    vaesenc xmm0, xmm0, [rdx+144]   ; Round 9

    vaesenclast xmm0, xmm0, [rdx+160] ; Final round (round 10)
    vmovdqu [rdi], xmm0
    ret

; Optimized AES-256 key expansion
; rdi = output buffer (240 bytes for 15 round keys), rdx = 32-byte master key
aes256Keygen:
    ; Load master key
    vmovdqu xmm0, [rdx]      ; First 16 bytes
    vmovdqu xmm1, [rdx+16]   ; Second 16 bytes

    ; Store initial round keys
    vmovdqu [rdi], xmm0      ; Round 0 key
    vmovdqu [rdi+16], xmm1   ; Round 1 key

    ; Key expansion constants
    mov r8d, 1               ; RCON value
    mov r9, 32               ; Byte offset for storing keys
    mov r10, 13              ; Remaining rounds to generate

.keygenLoop:

    test r10, 1
    jz .evenRound

.oddRound:
    ; Pattern: SubWord(RotWord(W[i-1])) transformation with RCON
    vaeskeygenassist xmm2, xmm1, 0
    vpshufd xmm2, xmm2, 0xFF        ; Get the rotated/substituted value

    ; Manually apply RCON to the first byte
    vmovd xmm3, r8d
    vpslldq xmm3, xmm3, 12          ; Move RCON to position
    vpxor xmm2, xmm2, xmm3          ; Apply RCON

    ; Key expansion transformation for xmm0
    vpslldq xmm3, xmm0, 4           ; Shift left by 4 bytes
    vpxor xmm0, xmm0, xmm3
    vpslldq xmm3, xmm0, 4
    vpxor xmm0, xmm0, xmm3
    vpslldq xmm3, xmm0, 4
    vpxor xmm0, xmm0, xmm3
    vpxor xmm0, xmm0, xmm2          ; Final XOR with transformed value

    ; Update RCON for next iteration
    shl r8d, 1
    cmp r8d, 0x80                   ; Handle RCON overflow
    jne .storeOddKey
    mov r8d, 0x1B                   ; Reset RCON with polynomial

.storeOddKey:
    vmovdqu [rdi + r9], xmm0
    add r9, 16
    dec r10
    jz .keygenDone
    jmp .evenRound

.evenRound:
    ; Pattern: SubWord(W[i-1]) transformation (no rotation, no RCON)
    vaeskeygenassist xmm2, xmm0, 0
    vpshufd xmm2, xmm2, 0xAA        ; Get SubWord result

    ; Key expansion transformation for xmm1
    vpslldq xmm3, xmm1, 4
    vpxor xmm1, xmm1, xmm3
    vpslldq xmm3, xmm1, 4
    vpxor xmm1, xmm1, xmm3
    vpslldq xmm3, xmm1, 4
    vpxor xmm1, xmm1, xmm3
    vpxor xmm1, xmm1, xmm2

    vmovdqu [rdi + r9], xmm1
    add r9, 16
    dec r10
    jnz .keygenLoop

.keygenDone:
    ret

aesXtsEncrypt:
    test r8, r8
    jz .done

    vmovdqu xmm15, [rcx]

    mov r9, r8
    shr r9, 2
    and r8, 3

.process4blocks:
    test r9, r9
    jz .processRemaining

    vmovdqu xmm11, [rsi]
    vmovdqu xmm12, [rsi+16]
    vmovdqu xmm13, [rsi+32]
    vmovdqu xmm14, [rsi+48]

    ; Apply initial tweak XOR
    vpxor xmm11, xmm11, xmm15
    vpxor xmm12, xmm12, xmm15
    vpxor xmm13, xmm13, xmm15
    vpxor xmm14, xmm14, xmm15

    ; Load round 0 key and apply
    vmovdqu xmm0, [rdx]
    vpxor xmm11, xmm11, xmm0
    vpxor xmm12, xmm12, xmm0
    vpxor xmm13, xmm13, xmm0
    vpxor xmm14, xmm14, xmm0

    ; Rounds 1-13 for AES-256 (unrolled for maximum performance)
    vmovdqu xmm0, [rdx+16]   ; Round 1
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+32]   ; Round 2
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+48]   ; Round 3
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+64]   ; Round 4
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+80]   ; Round 5
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+96]   ; Round 6
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+112]  ; Round 7
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+128]  ; Round 8
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+144]  ; Round 9
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+160]  ; Round 10
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+176]  ; Round 11
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+192]  ; Round 12
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    vmovdqu xmm0, [rdx+208]  ; Round 13
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    ; Final round (round 14)
    vmovdqu xmm0, [rdx+224]
    vaesenclast xmm11, xmm11, xmm0
    vaesenclast xmm12, xmm12, xmm0
    vaesenclast xmm13, xmm13, xmm0
    vaesenclast xmm14, xmm14, xmm0

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
    add rsi, 64
    add rdi, 64
    dec r9
    jnz .process4blocks

.processRemaining:
    ; Handle remaining 1-3 blocks with optimized single-block processing
    test r8, r8
    jz .done

    ; Pre-load all round keys for remaining blocks to minimize memory access
    vmovdqu xmm0, [rdx]      ; Round 0
    vmovdqu xmm1, [rdx+16]   ; Round 1
    vmovdqu xmm2, [rdx+32]   ; Round 2
    vmovdqu xmm3, [rdx+48]   ; Round 3
    vmovdqu xmm4, [rdx+64]   ; Round 4
    vmovdqu xmm5, [rdx+80]   ; Round 5
    vmovdqu xmm6, [rdx+96]   ; Round 6
    vmovdqu xmm7, [rdx+112]  ; Round 7
    vmovdqu xmm8, [rdx+128]  ; Round 8
    vmovdqu xmm9, [rdx+144]  ; Round 9
    vmovdqu xmm10, [rdx+160] ; Round 10
    ; Note: xmm11-14 will be used for remaining rounds as needed

.remainingLoop:
    vmovdqu xmm11, [rsi]
    vpxor xmm11, xmm11, xmm15    ; Apply tweak

    ; Full AES-256 encryption with pre-loaded keys
    vpxor xmm11, xmm11, xmm0     ; Round 0
    vaesenc xmm11, xmm11, xmm1   ; Round 1
    vaesenc xmm11, xmm11, xmm2   ; Round 2
    vaesenc xmm11, xmm11, xmm3   ; Round 3
    vaesenc xmm11, xmm11, xmm4   ; Round 4
    vaesenc xmm11, xmm11, xmm5   ; Round 5
    vaesenc xmm11, xmm11, xmm6   ; Round 6
    vaesenc xmm11, xmm11, xmm7   ; Round 7
    vaesenc xmm11, xmm11, xmm8   ; Round 8
    vaesenc xmm11, xmm11, xmm9   ; Round 9
    vaesenc xmm11, xmm11, xmm10  ; Round 10

    ; Load remaining round keys on-demand to save registers
    vaesenc xmm11, xmm11, [rdx+176]  ; Round 11
    vaesenc xmm11, xmm11, [rdx+192]  ; Round 12
    vaesenc xmm11, xmm11, [rdx+208]  ; Round 13
    vaesenclast xmm11, xmm11, [rdx+224]  ; Round 14

    vpxor xmm11, xmm11, xmm15    ; Apply final tweak
    vmovdqu [rdi], xmm11

    add rsi, 16
    add rdi, 16
    dec r8
    jnz .remainingLoop

.done:
    ret