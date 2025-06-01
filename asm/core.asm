global avx
section .text
avx:
        vmovaps ymm0, [rdi]
        vmovaps ymm1, [rsi]
        vmovaps ymm2, [rdx]
        vfmadd132ps ymm0, ymm1, ymm2
        vmovaps [rcx], ymm0
        ret