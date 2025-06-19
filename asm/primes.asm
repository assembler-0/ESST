global primes
section .text

; Extremely intensive prime factorization + cryptographic operations
; This will absolutely destroy CPU performance
primes:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    xor rcx, rcx                    ; Step counter
    mov r8, rdi                     ; Original number

    ; If number is too small, make it much larger and more complex
    cmp rdi, 100
    jae .start_factorization

    ; Generate a large composite number from the input
    mov rax, rdi
    imul rax, 982451653             ; Large prime
    add rax, 1000000007             ; Another large prime
    imul rax, rdi                   ; Make it depend on input
    add rax, 2147483647             ; Mersenne prime
    mov rdi, rax

.start_factorization:
    mov r12, rdi                    ; Number to factor

.factorization_loop:
    ; === TRIAL DIVISION WITH EXTREME OPTIMIZATION RESISTANCE ===

    ; Check if r13 divides r12
    mov rax, r12
    xor rdx, rdx
    div r13                         ; Expensive division
    test rdx, rdx
    jnz .next_factor

    ; Found a factor! Do intensive verification
    mov r14, rax                    ; Store quotient
    mov r15, 100                    ; Verification iterations

.verify_factor:
    ; Multiply back to verify (prevent optimization)
    mov rax, r14
    imul rax, r13
    cmp rax, r12
    jne .verification_failed

    ; Additional verification with modular arithmetic
    mov rax, r12
    mov rbx, r13
    call .modular_verification

    dec r15
    jnz .verify_factor

    ; Factor verified, continue with quotient
    mov r12, r14
    add rcx, r13                    ; Add factor to step count

    ; Do expensive primality testing on the factor
    mov rdi, r13
    call .miller_rabin_test
    add rcx, rax                    ; Add primality test cost

    ; Reset to test same factor again
    mov r13, 2
    jmp .factorization_loop

.next_factor:
    ; === PRIME GENERATION WITH SIEVE OF ERATOSTHENES ===
    inc r13

    ; Skip even numbers except 2
    cmp r13, 2
    je .test_factor
    test r13, 1
    jz .next_factor

.test_factor:
    ; Expensive primality check for potential factors
    mov rdi, r13
    call .is_prime_expensive
    test rax, rax
    jz .next_factor

    ; Check if we've found all factors
    mov rax, r13
    imul rax, r13
    cmp rax, r12
    ja .factorization_complete

    ; Add computational cost
    inc rcx
    jmp .factorization_loop

.factorization_complete:
    ; If r12 > 1, it's a prime factor
    cmp r12, 1
    jle .done
    add rcx, r12

.done:
    ; Ensure minimum computational cost
    cmp rcx, 10000
    jae .store_result
    mov rcx, 10000

.store_result:
    mov [rsi], rcx

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.verification_failed:
    ; If verification fails, add penalty and continue
    add rcx, 1000
    jmp .next_factor

; Modular verification function (expensive)
.modular_verification:
    push rcx
    push rdx

    mov rcx, 50                     ; Expensive verification loops
.mod_loop:
    xor rdx, rdx
    div rbx
    imul rax, rbx
    add rax, rdx
    cmp rax, r12
    jne .mod_fail
    dec rcx
    jnz .mod_loop

    pop rdx
    pop rcx
    ret

.mod_fail:
    add rcx, 100                    ; Penalty for failed verification
    pop rdx
    pop rcx
    ret

; Extremely expensive primality test (Miller-Rabin style)
.miller_rabin_test:
    push rbx
    push rcx
    push rdx
    push r8
    push r9
    push r10

    xor rax, rax                    ; Return value (cost)

    ; Trivial cases
    cmp rdi, 2
    jb .mr_done
    je .mr_prime
    test rdi, 1
    jz .mr_done

    ; Find r and d such that n-1 = 2^r * d
    mov r8, rdi
    dec r8                          ; n-1
    xor r9, r9                      ; r = 0

.find_r:
    test r8, 1
    jnz .found_r
    shr r8, 1
    inc r9
    jmp .find_r

.found_r:
    ; r8 = d, r9 = r
    mov r10, 10                     ; Number of rounds (expensive)

.miller_rabin_round:
    ; Generate witness (simplified - use iteration count)
    mov rbx, r10
    add rbx, 2

    ; Compute a^d mod n using expensive exponentiation
    mov rcx, r8                     ; exponent = d
    mov rax, 1                      ; result = 1

.mod_exp:
    test rcx, 1
    jz .even_exp

    ; result = (result * base) mod n
    imul rax, rbx
    xor rdx, rdx
    div rdi
    mov rax, rdx                    ; rax = remainder

.even_exp:
    ; base = (base * base) mod n
    imul rbx, rbx
    mov r11, rbx
    xor rdx, rdx
    div rdi
    mov rbx, rdx

    shr rcx, 1
    jnz .mod_exp

    ; Check if a^d ≡ 1 (mod n)
    cmp rax, 1
    je .continue_round

    ; Check if a^d ≡ -1 (mod n)
    mov rdx, rdi
    dec rdx
    cmp rax, rdx
    je .continue_round

    ; Repeat r-1 times
    mov rcx, r9
    dec rcx

.repeat_square:
    test rcx, rcx
    jz .composite

    ; x = (x * x) mod n
    imul rax, rax
    xor rdx, rdx
    div rdi
    mov rax, rdx

    ; Check if x ≡ -1 (mod n)
    mov rdx, rdi
    dec rdx
    cmp rax, rdx
    je .continue_round

    dec rcx
    jmp .repeat_square

.composite:
    ; Composite found, add high cost
    add rax, 10000
    jmp .mr_done

.continue_round:
    add rax, 1000                   ; Cost per round
    dec r10
    jnz .miller_rabin_round

.mr_prime:
    add rax, 5000                   ; Cost for probable prime

.mr_done:
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rbx
    ret

; Expensive primality test with trial division
.is_prime_expensive:
    push rbx
    push rcx

    cmp rdi, 2
    jb .not_prime
    je .is_prime_yes
    test rdi, 1
    jz .not_prime

    ; Trial division up to sqrt(n)
    mov rbx, 3
    mov rax, rdi

.sqrt_loop:
    mov rcx, rax
    add rax, rdi
    shr rax, 1                      ; (rax + rdi/rax) / 2
    cmp rax, rcx
    jb .sqrt_loop

    mov rcx, rax                    ; sqrt(n)

.trial_division:
    cmp rbx, rcx
    ja .is_prime_yes

    mov rax, rdi
    xor rdx, rdx
    div rbx
    test rdx, rdx
    jz .not_prime

    add rbx, 2                      ; Next odd number
    jmp .trial_division

.is_prime_yes:
    mov rax, 1
    pop rcx
    pop rbx
    ret

.not_prime:
    xor rax, rax
    pop rcx
    pop rbx
    ret