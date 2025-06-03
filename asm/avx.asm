global avx
section .text
avx:
        ; Save registers
        push rbp
        mov rbp, rsp
        
        ; Initialize all YMM registers with different patterns to prevent optimization
        vmovaps ymm0, [rdi]
        vmovaps ymm1, [rsi] 
        vmovaps ymm2, [rdx]
        
        ; Create dependency chains and varied data patterns
        vbroadcastss ymm3, dword [rdi]
        vbroadcastss ymm4, dword [rsi+4]
        vbroadcastss ymm5, dword [rdx+8]
        vbroadcastss ymm6, dword [rdi+12]
        vbroadcastss ymm7, dword [rsi+16]
        vbroadcastss ymm8, dword [rdx+20]
        vbroadcastss ymm9, dword [rdi+24]
        vbroadcastss ymm10, dword [rsi+28]
        vbroadcastss ymm11, dword [rdx+32]
        vbroadcastss ymm12, dword [rdi+36]
        vbroadcastss ymm13, dword [rsi+40]
        vbroadcastss ymm14, dword [rdx+44]
        vbroadcastss ymm15, dword [rdi+48]
        
        ; MASSIVE ITERATION COUNT - CPU will suffer
        mov rax, 8192
        
loop:
        ; === WAVE 1: Maximum FMA dependency hell ===
        ; Create long dependency chains to prevent parallel execution
        vfmadd132ps ymm0, ymm15, ymm14    ; ymm0 = ymm0*ymm14 + ymm15 
        vfmadd132ps ymm1, ymm0, ymm13     ; ymm1 depends on ymm0
        vfmadd132ps ymm2, ymm1, ymm12     ; ymm2 depends on ymm1
        vfmadd132ps ymm3, ymm2, ymm11     ; ymm3 depends on ymm2
        vfmadd132ps ymm4, ymm3, ymm10     ; ymm4 depends on ymm3
        vfmadd132ps ymm5, ymm4, ymm9      ; ymm5 depends on ymm4
        vfmadd132ps ymm6, ymm5, ymm8      ; etc...
        vfmadd132ps ymm7, ymm6, ymm7
        
        ; === WAVE 2: Complex shuffles + FMA (memory bandwidth torture) ===
        vperm2f128 ymm8, ymm0, ymm1, 0x20
        vperm2f128 ymm9, ymm2, ymm3, 0x31
        vperm2f128 ymm10, ymm4, ymm5, 0x12
        vperm2f128 ymm11, ymm6, ymm7, 0x03
        vshufps ymm12, ymm8, ymm9, 0x88
        vshufps ymm13, ymm10, ymm11, 0xDD
        vfmadd231ps ymm14, ymm12, ymm13
        vfmsub231ps ymm15, ymm8, ymm9
        
        ; === WAVE 3: Transcendental function torture ===
        ; Reciprocal approximations (low accuracy = more work for CPU)
        vrcpps ymm0, ymm14
        vrcpps ymm1, ymm15
        vrcpps ymm2, ymm0    ; Nested reciprocals
        vrcpps ymm3, ymm1
        
        ; Square root approximations
        vrsqrtps ymm4, ymm2
        vrsqrtps ymm5, ymm3
        vrsqrtps ymm6, ymm4  ; Nested rsqrt
        vrsqrtps ymm7, ymm5
        
        ; Newton-Raphson refinement (maximum ALU torture)
        vmulps ymm8, ymm0, ymm0     ; x²
        vmulps ymm9, ymm8, ymm14    ; x²*a
        vsubps ymm10, ymm9, ymm1    ; x²*a - b
        vmulps ymm11, ymm10, ymm0   ; (x²*a - b)*x
        
        ; === WAVE 4: Division hell (slowest AVX operations) ===
        vdivps ymm12, ymm11, ymm6
        vdivps ymm13, ymm7, ymm12
        vdivps ymm14, ymm4, ymm13
        vdivps ymm15, ymm5, ymm14
        vdivps ymm0, ymm15, ymm8     ; Chain of divisions
        vdivps ymm1, ymm0, ymm9
        vdivps ymm2, ymm1, ymm10
        vdivps ymm3, ymm2, ymm11
        
        ; === WAVE 5: Mixed precision torture ===
        vcvtps2pd ymm4, xmm3         ; Convert to double (cross-domain penalty)
        vcvtpd2ps xmm5, ymm4         ; Convert back (more penalties)
        vinsertf128 ymm6, ymm6, xmm5, 1
        
        ; === WAVE 6: Gather operations (cache torture) ===
        ; Create index patterns that will thrash cache
        vpcmpeqd ymm7, ymm7, ymm7    ; All 1s
        vpsrld ymm8, ymm7, 25        ; Create indices
        vpslld ymm9, ymm8, 2         ; Scale for float indices
        
        ; === WAVE 7: Blend torture with varying masks ===
        vblendps ymm10, ymm0, ymm1, 0xAA
        vblendps ymm11, ymm2, ymm3, 0x55
        vblendps ymm12, ymm4, ymm6, 0xF0
        vblendps ymm13, ymm10, ymm11, 0x0F
        vblendps ymm14, ymm12, ymm13, 0xCC
        vblendps ymm15, ymm14, ymm0, 0x33
        
        ; === WAVE 8: Maximum FMA saturation ===
        vfmadd132ps ymm0, ymm15, ymm14
        vfmadd213ps ymm1, ymm0, ymm13
        vfmadd231ps ymm2, ymm1, ymm12
        vfmsub132ps ymm3, ymm2, ymm11
        vfmsub213ps ymm4, ymm3, ymm10
        vfmsub231ps ymm5, ymm4, ymm9
        vfnmadd132ps ymm6, ymm5, ymm8
        vfnmadd213ps ymm7, ymm6, ymm7
        vfnmadd231ps ymm8, ymm7, ymm6
        vfnmsub132ps ymm9, ymm8, ymm5
        vfnmsub213ps ymm10, ymm9, ymm4
        vfnmsub231ps ymm11, ymm10, ymm3
        
        ; === WAVE 9: Alternating add/sub (pipeline stall inducer) ===
        vfmaddsub132ps ymm12, ymm11, ymm2
        vfmsubadd132ps ymm13, ymm12, ymm1
        vfmaddsub213ps ymm14, ymm13, ymm0
        vfmsubadd213ps ymm15, ymm14, ymm15
        
        ; === WAVE 10: Comparison hell (predicate torture) ===
        vcmpps ymm0, ymm12, ymm13, 0x01  ; LT
        vcmpps ymm1, ymm14, ymm15, 0x02  ; LE  
        vcmpps ymm2, ymm0, ymm1, 0x04    ; NE
        vcmpps ymm3, ymm2, ymm12, 0x08   ; NLT
        
        ; Use comparison results to create branches in data flow
        vblendvps ymm4, ymm13, ymm14, ymm0
        vblendvps ymm5, ymm15, ymm12, ymm1
        vblendvps ymm6, ymm4, ymm5, ymm2
        vblendvps ymm7, ymm6, ymm3, ymm3
        
        ; === WAVE 11: Exponential computation approximation ===
        ; Polynomial approximation of e^x (extremely compute intensive)
        vmovaps ymm8, ymm7
        vmulps ymm9, ymm8, ymm8         ; x²
        vmulps ymm10, ymm9, ymm8        ; x³
        vmulps ymm11, ymm10, ymm8       ; x⁴
        vmulps ymm12, ymm11, ymm8       ; x⁵
        
        ; Combine with different coefficients
        vfmadd231ps ymm13, ymm8, [rdi]   ; + c1*x
        vfmadd231ps ymm13, ymm9, [rsi]   ; + c2*x²  
        vfmadd231ps ymm13, ymm10, [rdx]  ; + c3*x³
        vfmadd231ps ymm13, ymm11, [rdi]  ; + c4*x⁴
        vfmadd231ps ymm13, ymm12, [rsi]  ; + c5*x⁵
        
        ; === WAVE 12: Final torture with maximum register pressure ===
        vfmadd132ps ymm14, ymm13, ymm12
        vfmsub132ps ymm15, ymm14, ymm11
        vfnmadd132ps ymm0, ymm15, ymm10
        vfnmsub132ps ymm1, ymm0, ymm9
        vmulps ymm2, ymm1, ymm8
        vdivps ymm3, ymm2, ymm7        ; More expensive division at the end
        vrcpps ymm4, ymm3              ; Reciprocal of division result
        vsqrtps ymm5, ymm4             ; Square root of reciprocal
        
        ; Create data dependencies that prevent optimization
        vaddps ymm6, ymm5, ymm0
        vmulps ymm7, ymm6, ymm1
        vsubps ymm8, ymm7, ymm2
        vdivps ymm9, ymm8, ymm3
        
        ; Force CPU to keep all results "live"
        vmovaps ymm10, ymm9
        vaddps ymm11, ymm10, ymm4
        vmulps ymm12, ymm11, ymm5
        
        ; Loop with extremely high iteration count
        dec rax
        jnz loop
        
        ; Prevent compiler optimization by using results
        vaddps ymm0, ymm12, ymm0
        vmovaps [rdi], ymm0
        
        pop rbp
        ret