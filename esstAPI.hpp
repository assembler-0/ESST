#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    void sha256(long iterations);
    void p3np1E(unsigned long a, unsigned long * steps);
    void primes(unsigned long a, unsigned long * steps);
    void avx(float * a, float * b, float * c);
    void floodL1L2(void* buffer, unsigned long * iterations_ptr, size_t buffer1_size);
    void floodMemory(void* buffer, unsigned long * iterations_ptr, size_t buffer_size);
    void rowhammerAttack(void* buffer, unsigned long * iterations_ptr, size_t buffer_size);
    void floodNt(void * buffer, unsigned long * iterations_ptr, size_t buffer_size);
    void aes128EncryptBlock(void * out, const void * in, const void * key);
    void aes256Keygen(void* expanded_key);
    void aesXtsEncrypt(void * out, const void * in, const void* key, const void * tweak, size_t blocks);
    void aes128DecryptBlock(void * out, const void * in, const void * key);
    void aesXtsDecrypt(void * out, const void * in, const void* key, const void * tweak, size_t blocks);
    void startLZMA(int duration);

#ifdef __cplusplus
}
#endif
