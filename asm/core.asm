global avx
global p3np1E
section .text
avx:
        vmovaps ymm0, [rdi]
        vmovaps ymm1, [rsi]
        vmovaps ymm2, [rdx]
        vfmaddsub132ps ymm0, ymm1, ymm2
        vfmsubadd132ps ymm0, ymm1, ymm2
        vmovaps [rcx], ymm0
        ret
p3np1E:
