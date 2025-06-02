global avx
section .text
avx:

        ; Initialize all YMM registers with data
        vmovaps ymm0, [rdi]
        vmovaps ymm1, [rsi]
        vmovaps ymm2, [rdx]
        vmovaps ymm3, ymm0
        vmovaps ymm4, ymm1
        vmovaps ymm5, ymm2
        vmovaps ymm6, ymm0
        vmovaps ymm7, ymm1
        vmovaps ymm8, ymm2
        vmovaps ymm9, ymm0
        vmovaps ymm10, ymm1
        vmovaps ymm11, ymm2
        vmovaps ymm12, ymm0
        vmovaps ymm13, ymm1
        vmovaps ymm14, ymm2
        vmovaps ymm15, ymm0

.main_loop:
        ; WAVE 1: Parallel FMA operations (maximum throughput)
        vfmadd132ps ymm0, ymm1, ymm2
        vfmadd132ps ymm3, ymm4, ymm5
        vfmadd132ps ymm6, ymm7, ymm8
        vfmadd132ps ymm9, ymm10, ymm11
        vfmadd132ps ymm12, ymm13, ymm14
        vfmadd132ps ymm15, ymm0, ymm1
        vfmadd213ps ymm2, ymm3, ymm4
        vfmadd231ps ymm5, ymm6, ymm7

        ; WAVE 2: Mixed FMA variants for port saturation
        vfmsub132ps ymm8, ymm9, ymm10
        vfmsub213ps ymm11, ymm12, ymm13
        vfmsub231ps ymm14, ymm15, ymm0
        vfnmadd132ps ymm1, ymm2, ymm3
        vfnmadd213ps ymm4, ymm5, ymm6
        vfnmadd231ps ymm7, ymm8, ymm9
        vfnmsub132ps ymm10, ymm11, ymm12
        vfnmsub213ps ymm13, ymm14, ymm15

        ; WAVE 3: Complex alternating FMA/FMSUB
        vfmaddsub132ps ymm0, ymm1, ymm2
        vfmsubadd132ps ymm3, ymm4, ymm5
        vfmaddsub213ps ymm6, ymm7, ymm8
        vfmsubadd213ps ymm9, ymm10, ymm11
        vfmaddsub231ps ymm12, ymm13, ymm14
        vfmsubadd231ps ymm15, ymm0, ymm1

        ; WAVE 4: Intensive reciprocal and square root operations
        vrcpps ymm2, ymm0
        vrcpps ymm3, ymm1
        vrcpps ymm4, ymm15
        vrcpps ymm5, ymm14
        vsqrtps ymm6, ymm2
        vsqrtps ymm7, ymm3
        vsqrtps ymm8, ymm4
        vsqrtps ymm9, ymm5

        ; WAVE 5: More FMA with reciprocals and square roots
        vfmadd132ps ymm10, ymm6, ymm7
        vfmadd132ps ymm11, ymm8, ymm9
        vfmadd132ps ymm12, ymm2, ymm3
        vfmadd132ps ymm13, ymm4, ymm5
        vfmsub132ps ymm14, ymm10, ymm11
        vfmsub132ps ymm15, ymm12, ymm13

        ; WAVE 6: Matrix-style operations with maximum register cycling
        vmulps ymm0, ymm6, ymm10
        vmulps ymm1, ymm7, ymm11
        vmulps ymm2, ymm8, ymm12
        vmulps ymm3, ymm9, ymm13
        vaddps ymm4, ymm0, ymm14
        vaddps ymm5, ymm1, ymm15
        vaddps ymm6, ymm2, ymm4
        vaddps ymm7, ymm3, ymm5

        ; WAVE 7: Intensive blending and permutation torture
        vsubps ymm8, ymm6, ymm7
        vsubps ymm9, ymm4, ymm5
        vmulps ymm10, ymm8, ymm9
        vdivps ymm11, ymm10, ymm6
        vdivps ymm12, ymm7, ymm8

        ; WAVE 8: More complex FMA chains
        vfmadd132ps ymm13, ymm11, ymm12
        vfmadd132ps ymm14, ymm10, ymm9
        vfmadd132ps ymm15, ymm8, ymm7
        vfmsub132ps ymm0, ymm13, ymm14
        vfmsub132ps ymm1, ymm15, ymm0

        ; WAVE 9: Additional reciprocal/sqrt torture
        vrcpps ymm2, ymm1
        vrcpps ymm3, ymm0
        vsqrtps ymm4, ymm2
        vsqrtps ymm5, ymm3
        vfmadd132ps ymm6, ymm4, ymm5
        vfmadd132ps ymm7, ymm2, ymm3

        ; WAVE 10: Final massive computation wave
        vfnmadd132ps ymm8, ymm6, ymm7
        vfnmsub132ps ymm9, ymm4, ymm5
        vfmaddsub132ps ymm10, ymm8, ymm9
        vfmsubadd132ps ymm11, ymm6, ymm7
        vmulps ymm12, ymm10, ymm11
        vaddps ymm13, ymm12, ymm8
        vsubps ymm14, ymm13, ymm9
        vdivps ymm15, ymm14, ymm10

        ret