#pragma once
#include <cstddef>
extern "C" {
    void p3np1E(unsigned long a, unsigned long * steps);
    void avx(float * a, float * b, float * c, float * out);
    void floodL1L2(void* buffer, unsigned long* iterations_ptr, size_t buffer_size);
    void floodMemory(void* buffer, unsigned long* iterations_ptr, size_t buffer_size);
    void rowhammerAttack(void* buffer, unsigned long* iterations_ptr, size_t buffer_size);
    void floodNt(void* buffer, unsigned long* iterations_ptr, size_t buffer_size);
}