section .text
global aes128EncryptBlock, aes256Keygen, aesXtsEncrypt

; Windows x64 calling convention:
; RCX = first param (out)
; RDX = second param (in)
; R8 = third param (key)
; R9 = fourth param (tweak for aesXtsEncrypt)
; Stack for additional params

aes128EncryptBlock:
    ; void aes128EncryptBlock(void* out (RCX), const void* in (RDX), const void* key (R8))
    vmovdqu xmm0, [rdx]             ; Load plaintext
    vpxor xmm0, xmm0, [r8]          ; AddRoundKey (round 0)

    ; Rounds 1-9
    vaesenc xmm0, xmm0, [r8+16]     ; Round 1
    vaesenc xmm0, xmm0, [r8+32]     ; Round 2
    vaesenc xmm0, xmm0, [r8+48]     ; Round 3
    vaesenc xmm0, xmm0, [r8+64]     ; Round 4
    vaesenc xmm0, xmm0, [r8+80]     ; Round 5
    vaesenc xmm0, xmm0, [r8+96]     ; Round 6
    vaesenc xmm0, xmm0, [r8+112]    ; Round 7
    vaesenc xmm0, xmm0, [r8+128]    ; Round 8
    vaesenc xmm0, xmm0, [r8+144]    ; Round 9

    vaesenclast xmm0, xmm0, [r8+160] ; Final round (round 10)
    vmovdqu [rcx], xmm0              ; Store ciphertext
    ret

aes256Keygen:
    ; void aes256Keygen(void* expanded_key (RCX))
    ; Note: Windows calling convention passes the master key pointer differently
    ; You'll need to adjust this based on how you get the master key
    ; Assuming RCX points to where expanded keys should be stored
    ; and master key is at RCX+240 (adjust as needed)

    ; Load master key (adjust pointer as needed)
    vmovdqu xmm0, [rcx+240]      ; First 16 bytes
    vmovdqu xmm1, [rcx+256]      ; Second 16 bytes

    ; Store initial round keys
    vmovdqu [rcx], xmm0          ; Round 0 key
    vmovdqu [rcx+16], xmm1       ; Round 1 key

    ; Key expansion constants
    mov r10d, 1                  ; RCON value
    mov r11, 32                  ; Byte offset for storing keys
    mov r9, 13                   ; Remaining rounds to generate

.keygenLoop:
    test r9, 1
    jz .evenRound

.oddRound:
    vaeskeygenassist xmm2, xmm1, 0
    vpshufd xmm2, xmm2, 0xFF

    vmovd xmm3, r10d
    vpslldq xmm3, xmm3, 12
    vpxor xmm2, xmm2, xmm3

    vpslldq xmm3, xmm0, 4
    vpxor xmm0, xmm0, xmm3
    vpslldq xmm3, xmm0, 4
    vpxor xmm0, xmm0, xmm3
    vpslldq xmm3, xmm0, 4
    vpxor xmm0, xmm0, xmm3
    vpxor xmm0, xmm0, xmm2

    shl r10d, 1
    cmp r10d, 0x80
    jne .storeOddKey
    mov r10d, 0x1B

.storeOddKey:
    vmovdqu [rcx + r11], xmm0
    add r11, 16
    dec r9
    jz .keygenDone
    jmp .evenRound

.evenRound:
    vaeskeygenassist xmm2, xmm0, 0
    vpshufd xmm2, xmm2, 0xAA

    vpslldq xmm3, xmm1, 4
    vpxor xmm1, xmm1, xmm3
    vpslldq xmm3, xmm1, 4
    vpxor xmm1, xmm1, xmm3
    vpslldq xmm3, xmm1, 4
    vpxor xmm1, xmm1, xmm3
    vpxor xmm1, xmm1, xmm2

    vmovdqu [rcx + r11], xmm1
    add r11, 16
    dec r9
    jnz .keygenLoop

.keygenDone:
    ret

aesXtsEncrypt:
    ; void aesXtsEncrypt(void* out (RCX), const void* in (RDX),
    ;                    const void* key (R8), const void* tweak (R9),
    ;                    size_t blocks (stack))
    ; Get blocks from stack (5th parameter in Windows x64)
    mov rax, [rsp+40]           ; 32 bytes shadow space + 8 bytes return address

    test rax, rax
    jz .done

    vmovdqu xmm15, [r9]         ; Load tweak

    mov r10, rax
    shr r10, 2
    and rax, 3

.process4blocks:
    test r10, r10
    jz .processRemaining

    vmovdqu xmm11, [rdx]
    vmovdqu xmm12, [rdx+16]
    vmovdqu xmm13, [rdx+32]
    vmovdqu xmm14, [rdx+48]

    ; Apply initial tweak XOR
    vpxor xmm11, xmm11, xmm15
    vpxor xmm12, xmm12, xmm15
    vpxor xmm13, xmm13, xmm15
    vpxor xmm14, xmm14, xmm15

    ; Load round 0 key and apply
    vmovdqu xmm0, [r8]
    vpxor xmm11, xmm11, xmm0
    vpxor xmm12, xmm12, xmm0
    vpxor xmm13, xmm13, xmm0
    vpxor xmm14, xmm14, xmm0

    ; Rounds 1-13 (truncated for brevity - same as original)
    vmovdqu xmm0, [r8+16]   ; Round 1
    vaesenc xmm11, xmm11, xmm0
    vaesenc xmm12, xmm12, xmm0
    vaesenc xmm13, xmm13, xmm0
    vaesenc xmm14, xmm14, xmm0

    ; ... (include all rounds as in original)

    ; Final round (round 14)
    vmovdqu xmm0, [r8+224]
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
    vmovdqu [rcx], xmm11
    vmovdqu [rcx+16], xmm12
    vmovdqu [rcx+32], xmm13
    vmovdqu [rcx+48], xmm14

    ; Update pointers
    add rdx, 64
    add rcx, 64
    dec r10
    jnz .process4blocks

.processRemaining:
    test rax, rax
    jz .done

    ; Pre-load all round keys
    vmovdqu xmm0, [r8]      ; Round 0
    vmovdqu xmm1, [r8+16]   ; Round 1
    ; ... (load all rounds as in original)

.remainingLoop:
    vmovdqu xmm11, [rdx]
    vpxor xmm11, xmm11, xmm15

    ; Full AES-256 encryption
    vpxor xmm11, xmm11, xmm0
    vaesenc xmm11, xmm11, xmm1
    ; ... (all rounds as in original)

    vpxor xmm11, xmm11, xmm15
    vmovdqu [rcx], xmm11

    add rdx, 16
    add rcx, 16
    dec rax
    jnz .remainingLoop

.done:
    ret