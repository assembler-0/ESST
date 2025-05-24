global sse
section .text
sse:
        movaps xmm0, [rdi]      ; load 4 floats from ptr1
        movaps xmm1, [rsi]      ; load 4 floats from ptr2
        addps  xmm0, xmm1       ; xmm0 = xmm0 + xmm1
        movaps [rdx], xmm0      ; store result to output ptr
        ret
