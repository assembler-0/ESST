global avx
section .text
avx:
        ;DATA
        vmovaps ymm0, [rdi]
        vmovaps ymm1, [rsi]
        vmovaps ymm2, [rdx]
        vmovaps ymm3, ymm0
        vmovaps ymm4, ymm1
        vmovaps ymm5, ymm2
        ;ITERATIONS1
        mov rax, 1000
        ;FMA
        vfmadd132ps ymm0, ymm1, ymm2
        vfmadd132ps ymm3, ymm4, ymm5
        vfmadd132ps ymm4, ymm0, ymm3
        vfmadd132ps ymm2, ymm3, ymm1
        ;RECIPROCAL
        vrcpps ymm6, ymm0
        vrcpps ymm7, ymm4
        ;SQRT
        vsqrtps ymm8, ymm6
        vsqrtps ymm9, ymm7
        ;MATRIX
        vmulps ymm10, ymm0, ymm3
        vmulps ymm11, ymm1, ymm4
        vaddps ymm10, ymm10, ymm6
        vaddps ymm11, ymm11, ymm7
        vfmaddsub132ps ymm8, ymm1, ymm2
        vfmsubadd132ps ymm9, ymm1, ymm2
        vmovaps [rcx], ymm0
        ret