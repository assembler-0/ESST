global sse
section .text
sse:
        vmovaps ymm0, [rdi]
        vmovaps ymm1, [rsi]
        vmulps  ymm0, ymm1
        vmovaps [rdx], ymm0
        ret