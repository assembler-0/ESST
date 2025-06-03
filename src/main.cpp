#include "core.hpp"
#include "pcg_random.hpp"
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <chrono>
#include <vector>
#include <immintrin.h>
#include <cpuid.h>
#include <sched.h>
#include <sys/mman.h>
class esst {
public:
    void init() {
        detect_cpu_features();
        std::cout << "ESST version " << APP_VERSION << " | CPU: " << cpu_brand << "\n";
        std::cout << "Features: AVX" << (has_avx ? "+" : "-")
                  << " | AVX2" << (has_avx2 ? "+" : "-")
                  << " | FMA" << (has_fma ? "+" : "-") << "\n";

        while (running) {
            std::cout << "[ESST] >> ";
            if (!std::getline(std::cin, op_mode)) break;

            if (auto it = command_map.find(op_mode); it != command_map.end()) {
                it->second();
            }
            else if (!op_mode.empty()) {
                std::cout << "Invalid command\n";
            }
        }
    }

private:
    bool running = true;
    std::string op_mode;
    std::string cpu_brand;
    bool has_avx = false, has_avx2 = false, has_fma = false;
    const unsigned int num_threads = std::thread::hardware_concurrency();

    static constexpr auto APP_VERSION = "0.4";
    static constexpr int AVX_BUFFER_SIZE = 64; // 256 bytes (L1 cache line optimized)
    static constexpr int COLLATZ_BATCH_SIZE = 100000;

    const std::unordered_map<std::string, std::function<void()>> command_map = {
        {"exit", [this]() { running = false; }},
        {"menu", [this]() { showMenu(); }},
        {"avx",  [this]() { initAvx(); }},
        {"3np1", [this]() { init3np1(); }},
        {"disk", [this]() { initDiskWrite(); }},
        {"full", [this]() { nuclearOption(); }},
        {"mem", [this]() { initMem(); }},
        {"aesenc", [this]() { initAESENC(); }},
        {"aesdec", [this]() { initAESDEC(); }}
    };

    void detect_cpu_features() {
        char brand[0x40] = {0};
        unsigned int eax, ebx, ecx, edx;

        __get_cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
        memcpy(brand, &eax, 4); memcpy(brand+4, &ebx, 4);
        memcpy(brand+8, &ecx, 4); memcpy(brand+12, &edx, 4);

        __get_cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
        memcpy(brand+16, &eax, 4); memcpy(brand+20, &ebx, 4);
        memcpy(brand+24, &ecx, 4); memcpy(brand+28, &edx, 4);

        __get_cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
        memcpy(brand+32, &eax, 4); memcpy(brand+36, &ebx, 4);
        memcpy(brand+40, &ecx, 4); memcpy(brand+44, &edx, 4);

        cpu_brand = brand;

        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
        has_avx = ecx & bit_AVX;
        has_fma = ecx & bit_FMA;

        __get_cpuid(7, &eax, &ebx, &ecx, &edx);
        has_avx2 = ebx & bit_AVX2;
    }

    static void showMenu() {
        std::cout << "\n========= ESST =========\n"
                  << "Extreme System Stability Test\n"
                  << "avx   - AVX/FMA Stress Test\n"
                  << "3np1  - Collatz Conjecture bruteforce\n"
                  << "mem   - Extreme memory testing\n"
                  << "aesenc   - Vetor AES Encrypt stressing\n"
                  << "aesdec   - Vetor AES Decrypt stressing\n"
                  << "disk   - Disk stressing\n"
                  << "full  - Combined AVX+Collatz+Mem+Aes Full System Stress\n"
                  << "exit  - Exit Program\n\n";
    }

    void init3np1() {
        auto [iterations, lower, upper] = getInputs("Collatz");
        if (iterations == 0) return;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        const auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([=]() { collatzWorker(iterations, lower, upper, i); });
        }

        for (auto& t : threads) t.join();

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total Collatz time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    void initAvx() {
        auto [iterations, lower, upper] = getInputs("AVX");
        if (iterations == 0) return;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        const auto start = std::chrono::high_resolution_clock::now();

        for (unsigned i = 0; i < num_threads; ++i) {
            threads.emplace_back([=]() { avxWorker(iterations, lower, upper, i); });
        }

        for (auto& t : threads) t.join();

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total AVX compute time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    void initMem() {
        char status;
        std::cout << "ONE TIME WARNING, THIS TEST CONTAINS ROWHAMMER ATTACK, PROCEED? (yY/nN): ";
        std::cin >> status;
        switch (status){
            case 'y': break;
            case 'Y': break;
            default: return;
        }
        unsigned long iterations = 0;
        std::cout << "Iterations?: ";
        if (!(std::cin >> iterations)) badInput();
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        const auto start = std::chrono::high_resolution_clock::now();
        for (unsigned i = 0; i < num_threads; ++i) {
            threads.emplace_back([=]() { memoryWorker(iterations, i); });
        }

        for (auto& t : threads) t.join();

        const auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total memory test time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    void initAESENC() {
        unsigned long iterations = 0;
        unsigned int block_size = 0;
        std::cout << "Iterations?: ";
        if (!(std::cin >> iterations)) badInput();
        if (iterations > 100) std::cout << "Over 100 iterations is not recommended, continuing...\n";
        std::cout << "Block size? (LEAVE 24 IF YOU DON'T KNOW WHAT YOU ARE DOING): ";
        if (!(std::cin >> block_size)) badInput();
        std::vector<std::thread> threads;
        const auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(aesENCWorker,iterations, i, block_size);
        }
        for (auto& t : threads) t.join();
        const auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total AES Encrypt compute time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms\n";
    }

    void initAESDEC() {
        unsigned long iterations = 0;
        unsigned int block_size = 0;
        std::cout << "Iterations?: ";
        if (!(std::cin >> iterations)) badInput();
        if (iterations > 100) std::cout << "Over 100 iterations is not recommended, continuing...\n";
        std::cout << "Block size? (LEAVE 24 IF YOU DON'T KNOW WHAT YOU ARE DOING): ";
        if (!(std::cin >> block_size)) badInput();
        std::vector<std::thread> threads;
        const auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(aesDECWorker,iterations, i, block_size);
        }
        for (auto& t : threads) t.join();
        const auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total AES Decrypt compute time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms\n";
    }

    void initDiskWrite(){
        unsigned long iterations = 0;
        std::cout << "Iterations?: ";
        if (!(std::cin >> iterations)) badInput();
        if (iterations > 5) std::cout << "Over 5 iterations is not generally recommended, continuing...\n";
        std::vector<std::thread> threads;
        const auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back(diskWriteWorker, iterations, i);
        }
        for (auto& t : threads) t.join();
        const auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total Disk Write time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << " ms\n";
    }

    void nuclearOption() {
        std::cout << "Launching full stress test (AVX + Collatz + AES + Mem + Disk)...\n";
        constexpr unsigned long nuke_iterations = 100000000;
        constexpr unsigned long nuke_iterations_aes = 10;
        constexpr unsigned long nuke_iterations_disk = 5;
        constexpr unsigned long nuke_iterations_mem = 10;
        constexpr unsigned long lower_avx = 0.0001, upper_avx = 1000000000000000;
        constexpr unsigned long lower = 1, upper = 1000000000000000;
        const int block_size = 24;
        std::vector<std::thread> threads;
        threads.reserve(num_threads * 5);

        const auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_threads; ++i){
            threads.emplace_back([=]() { avxWorker(nuke_iterations, lower_avx, upper_avx, i); });
            threads.emplace_back([=]() { collatzWorker(nuke_iterations, lower, upper, i); });
            threads.emplace_back([=]() { memoryWorker(nuke_iterations_mem, i); });
            threads.emplace_back([=]() { aesENCWorker(nuke_iterations_aes, i, block_size); });
            threads.emplace_back([=]() { aesDECWorker(nuke_iterations_aes, i, block_size); });
            threads.emplace_back([=]() { diskWriteWorker(nuke_iterations_disk, i); });
        }
        for (auto& t : threads) t.join();
        const auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Full test complete! Time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    std::tuple<unsigned long, unsigned long, unsigned long> getInputs(const std::string& test) {
        unsigned long its = 0;
        float lower = 0, upper = 0;

        std::cout << test << " iterations? : ";
        if (!(std::cin >> its)) { badInput(); return {0,0,0}; }

        std::cout << "Lower limit? : ";
        if (!(std::cin >> lower)) { badInput(); return {0,0,0}; }

        std::cout << "Upper limit? : ";
        if (!(std::cin >> upper)) {
            badInput();
            return {0,0,0};
        }

        // Now check range separately
        if (lower > upper) {
            std::cout << "Error: Lower limit cannot be greater than upper limit.\n";
            badInput();
            return {0,0,0};
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return {its, lower, upper};
    }

    static void badInput() {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid input!\n";
    }

    static void pinThread(int core) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core % std::thread::hardware_concurrency(), &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    }
    static void* allocate_huge_buffer(size_t size) {
    #ifdef __linux__
        void* ptr = mmap(nullptr, size, PROT_READ|PROT_WRITE,
                        MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
        if (ptr != MAP_FAILED) return ptr;
    #endif
        return aligned_alloc(1 << 21, size); // Fallback to 2MB aligned
    }

    static void free_buffer(void* buf, size_t size) {
    #ifdef __linux__
        munmap(buf, size);
    #else
        free(buf);
    #endif
    }

    static void memoryWorker(unsigned long iterations, const int thread_id) {
        pinThread(thread_id);
        constexpr size_t size = 1 << 30;
        void* buffer = allocate_huge_buffer(size);
        constexpr size_t buffer_size = size;
        floodL1L2(buffer, &iterations, buffer_size);
        floodMemory(buffer, &iterations, buffer_size);
        floodNt(buffer, &iterations, buffer_size);
        rowhammerAttack(buffer, &iterations, buffer_size);
        free_buffer(buffer, size);
    }

    static void aesENCWorker(const long iterations, int tid, const int block_size) {
        pinThread(tid);
        const auto start = std::chrono::high_resolution_clock::now();
        // Allocate aligned buffers
        alignas(16) uint8_t key[32] = {0x01}; // All-zero key (worst-case)
        alignas(16) uint8_t expanded_key[240]; // AES-256 expanded key
        alignas(16) uint8_t plaintext[16] = {0};
        alignas(16) uint8_t ciphertext[16];
        const size_t BLOCKS = 1 << block_size;
        auto buffer = std::make_unique<uint8_t[]>(BLOCKS * 16);
        pcg32 gen(std::random_device{}());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& v : key) v = dist(gen);
        for (long i = 0; i < iterations; i++){
            // Key expansion (stress FPU)
            aes256Keygen(expanded_key);
            // Encrypt individual blocks (stress latency)
            for (size_t i = 0; i < BLOCKS; i++) {
                aes128EncryptBlock(ciphertext, plaintext, key);
                asm volatile("" : : "r"(ciphertext) : "memory");
            }
            // XTS mode (stress throughput)
            uint8_t tweak[16] = {0};
            aesXtsEncrypt(buffer.get(), buffer.get(), expanded_key, tweak, BLOCKS);
        }
        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "AES Encrypt Thread " << tid << " done in: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    static void aesDECWorker(const long iterations, int tid, const int block_size) {
        pinThread(tid);
        const auto start = std::chrono::high_resolution_clock::now();
        // Allocate aligned buffers
        alignas(16) uint8_t key[32] = {0x01}; // All-zero key (worst-case)
        alignas(16) uint8_t expanded_key[240]; // AES-256 expanded key
        alignas(16) uint8_t ciphertext[16] = {0};
        alignas(16) uint8_t plaintext[16];
        const size_t BLOCKS = 1 << block_size;
        auto buffer = std::make_unique<uint8_t[]>(BLOCKS * 16);
        pcg32 gen(std::random_device{}());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& v : key) v = dist(gen);

        for (long i = 0; i < iterations; i++){
            // Key expansion (stress FPU)
            aes256Keygen(expanded_key);
            // Decrypt individual blocks (stress latency)
            for (size_t i = 0; i < BLOCKS; i++) {
                aes128DecryptBlock(plaintext, ciphertext, key);
                asm volatile("" : : "r"(plaintext) : "memory");
            }
            // XTS mode decryption (stress throughput)
            uint8_t tweak[16] = {0};
            aesXtsDecrypt(buffer.get(), buffer.get(), expanded_key, tweak, BLOCKS);
        }
        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "AES Decrypt Thread " << tid << " done in: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    static void collatzWorker(unsigned long iterations, unsigned long lower, unsigned long upper, int tid) {
        pinThread(tid);
        pcg32 gen(42u + tid, 54u + tid);
        std::uniform_int_distribution<unsigned long> dist(lower, upper);

        unsigned long total_steps = 0;
        const auto start = std::chrono::high_resolution_clock::now();

        for (unsigned long i = 0; i < iterations; ) {
            const unsigned long batch = std::min(static_cast<unsigned long>(COLLATZ_BATCH_SIZE), iterations - i);
            unsigned long batch_steps = 0;

            for (unsigned long j = 0; j < batch; ++j) {
                unsigned long steps = 0;
                p3np1E(dist(gen), &steps);
                batch_steps += steps;
            }

            total_steps += batch_steps;
            i += batch;
        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Collatz Thread " << tid << " done: " << total_steps << " steps in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    static void avxWorker(unsigned long iterations, float lower, float upper, int tid) {
        pinThread(tid);
        pcg32 gen(42u + tid, 54u + tid);
        std::uniform_real_distribution<float> dist(lower, upper);

        alignas(32) float n1[AVX_BUFFER_SIZE], n2[AVX_BUFFER_SIZE], n3[AVX_BUFFER_SIZE];

        const auto start = std::chrono::high_resolution_clock::now();

        for (unsigned long i = 0; i < iterations; ++i) {
            for (int j = 0; j < AVX_BUFFER_SIZE; ++j) {
                n1[j] = dist(gen);
                n2[j] = dist(gen);
                n3[j] = dist(gen);
            }

            for (int offset = 0; offset < AVX_BUFFER_SIZE; offset += 8) {
                avx(n1+offset, n2+offset, n3+offset);
            }

        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "AVX Thread " << tid << " done in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }
    static void diskWriteWorker(unsigned long iterations, int tid){
        pinThread(tid);
        std::string filename = "/tmp/writeTestThread" + std::to_string(tid) + ".bin";
        for (int i = 0; i < iterations; ++i) {
            diskWrite(filename.c_str());
        }
        std::cout << "Disk Write thread " << tid << " done\n";
    }
};

int main() {
    esst().init();
    return 0;
}
