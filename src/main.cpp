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
#include <atomic>
#include <numa.h>

class acts {
public:
    void init() {
        detect_cpu_features();
        init_numa();
        std::cout << "ACTS version " << APP_VERSION << " | CPU: " << cpu_brand << "\n";
        std::cout << "Features: AVX" << (has_avx ? "+" : "-")
                  << " | AVX2" << (has_avx2 ? "+" : "-")
                  << " | FMA" << (has_fma ? "+" : "-")
                  << " | AVX512" << (has_avx512 ? "+" : "-") << "\n";
        std::cout << "NUMA nodes: " << numa_nodes << " | Threads: " << num_threads << "\n";

        while (running) {
            std::cout << "[ACTS] >> ";
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
    bool has_avx = false, has_avx2 = false, has_fma = false, has_avx512 = false;
    int numa_nodes = 1;
    const unsigned int num_threads = std::thread::hardware_concurrency();

    static constexpr auto APP_VERSION = "0.3";
    static constexpr int AVX_BUFFER_SIZE = 256; // Increased for better vectorization
    static constexpr int COLLATZ_BATCH_SIZE = 1000000; // 10x larger batches
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t L3_CACHE_SIZE = 32 * 1024 * 1024; // 32MB assumption

    const std::unordered_map<std::string, std::function<void()>> command_map = {
        {"exit", [this]() { running = false; }},
        {"menu", [this]() { showMenu(); }},
        {"avx",  [this]() { initAvx(); }},
        {"3np1", [this]() { init3np1(); }},
        {"nuke", [this]() { nuclearOption(); }},
        {"mem", [this]() { initMem(); }},
        {"aes", [this]() { initAES(); }}
    };

    void detect_cpu_features() {
        char brand[0x40] = {0};
        unsigned int eax, ebx, ecx, edx;

        // CPU brand detection (unchanged)
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
        has_avx512 = ebx & (1 << 16); // AVX512F
    }

    void init_numa() {
        #ifdef __linux__
        if (numa_available() != -1) {
            numa_nodes = numa_max_node() + 1;
        }
        #endif
    }

    static void showMenu() {
        std::cout << "\n========= ACTS =========\n"
                  << "avx   - AVX/FMA Stress Test\n"
                  << "3np1  - Collatz Conjecture Bruteforce\n"
                  << "mem   - Extreme memory testing\n"
                  << "aes   - Vector AES stressing\n"
                  << "nuke  - Combined AVX+Collatz+Mem Full System Stress\n"
                  << "exit  - Exit Program\n\n";
    }

    void init3np1() {
        auto [iterations, lower, upper] = getInputs("Collatz");
        if (iterations == 0) return;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        // Use atomic for better progress tracking
        std::atomic<uint64_t> total_steps{0};
        std::atomic<uint64_t> completed_iterations{0};

        const auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([=, &total_steps, &completed_iterations]() {
                collatzWorker(iterations, lower, upper, i, total_steps, completed_iterations);
            });
        }

        for (auto& t : threads) t.join();

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total Collatz time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms | Total steps: " << total_steps.load() << "\n";
    }

    void initAvx() {
        auto [iterations, lower, upper] = getInputs("AVX");
        if (iterations == 0) return;

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        const auto start = std::chrono::high_resolution_clock::now();

        for (unsigned i = 0; i < num_threads; ++i) {
            threads.emplace_back([=]() {
                if (has_avx512) {
                    avx512Worker(iterations, lower, upper, i);
                } else {
                    avxWorker(iterations, lower, upper, i);
                }
            });
        }

        for (auto& t : threads) t.join();

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total AVX time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    void initMem() {
        char status;
        std::cout << "ONE TIME WARNING THIS TEST CONTAINS ROWHAMMER ATTACK, PROCEED? (yY/nN): ";
        std::cin >> status;
        switch (status){
            case 'y': case 'Y': break;
            default: return;
        }

        unsigned long iterations = 0;
        std::cout << "Iterations?: ";
        if (!(std::cin >> iterations)) { badInput(); return; }

        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        const auto start = std::chrono::high_resolution_clock::now();
        for (unsigned i = 0; i < num_threads; ++i) {
            threads.emplace_back([=]() { memoryWorkerOptimized(iterations, i); });
        }

        for (auto& t : threads) t.join();

        const auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Total memory test time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    void initAES() {
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([=]() { aesWorkerOptimized(i); });
        }
        for (auto& t : threads) t.join();
        std::cout << "AES stress test completed\n";
    }

    void nuclearOption() {
        std::cout << "Launching nuclear stress test (AVX + Collatz + Mem + AES)...\n";
        constexpr unsigned long nuke_iterations = 10000000000UL; // 10x more iterations
        constexpr int lower = 1, upper = 10000000; // Larger range

        std::vector<std::thread> threads;
        threads.reserve(num_threads * 4);
        std::atomic<uint64_t> total_steps{0};
        std::atomic<uint64_t> completed_iterations{0};

        const auto start = std::chrono::high_resolution_clock::now();

        // Create 4 different workloads per core for maximum stress
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([=, &total_steps, &completed_iterations]() {
                if (has_avx512) avx512Worker(nuke_iterations, lower, upper, i);
                else avxWorker(nuke_iterations, lower, upper, i);
            });
            threads.emplace_back([=, &total_steps, &completed_iterations]() {
                collatzWorker(nuke_iterations, lower, upper, i, total_steps, completed_iterations);
            });
            threads.emplace_back([=]() { memoryWorkerOptimized(nuke_iterations, i); });
            threads.emplace_back([=]() { aesWorkerOptimized(i); });
        }

        for (auto& t : threads) t.join();

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Nuclear test complete! Time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms | Total Collatz steps: " << total_steps.load() << "\n";
    }

    // Optimized memory allocation with NUMA awareness
    static void* allocate_numa_buffer(size_t size, int node = -1) {
        #ifdef __linux__
        void* ptr = nullptr;

        // Try huge pages first
        if (node >= 0) {
            numa_set_preferred(node);
        }

        ptr = mmap(nullptr, size, PROT_READ|PROT_WRITE,
                  MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);

        if (ptr != MAP_FAILED) {
            // Prefault pages for better performance
            memset(ptr, 0, size);
            return ptr;
        }
        #endif

        // Fallback to aligned allocation
        ptr = aligned_alloc(1 << 21, size);
        if (ptr) memset(ptr, 0, size);
        return ptr;
    }

    // Cache-optimized memory worker
    static void memoryWorkerOptimized(unsigned long iterations, const int thread_id) {
        pinThreadOptimal(thread_id);

        constexpr size_t size = 2UL << 30; // 4GB per thread
        int numa_node = thread_id % numa_max_node();
        void* buffer = allocate_numa_buffer(size, numa_node);

        if (!buffer) {
            std::cout << "Failed to allocate buffer for thread " << thread_id << "\n";
            return;
        }

        // Multiple memory stress patterns
        floodL1L2Optimized(buffer, iterations, size);
        floodMemoryOptimized(buffer, iterations, size);
        floodNtOptimized(buffer, iterations, size);

        std::cout << "WARNING: Rowhammer test running on thread " << thread_id
                  << " (may cause bit flips)\n";
        rowhammerAttackOptimized(buffer, iterations, size);

        free_buffer(buffer, size);
    }

    // AVX-512 worker for modern CPUs
    static void avx512Worker(unsigned long iterations, float lower, float upper, int tid) {
        pinThreadOptimal(tid);
        pcg32 gen(42u + tid, 54u + tid);
        std::uniform_real_distribution<float> dist(lower, upper);

        constexpr int AVX512_SIZE = 512; // 16 floats * 32 sets
        alignas(64) float n1[AVX512_SIZE], n2[AVX512_SIZE],
                          n3[AVX512_SIZE], out[AVX512_SIZE];

        const auto start = std::chrono::high_resolution_clock::now();

        for (unsigned long i = 0; i < iterations; ++i) {
            // Vectorized random number generation
            for (int j = 0; j < AVX512_SIZE; j += 16) {
                for (int k = 0; k < 16; ++k) {
                    n1[j+k] = dist(gen);
                    n2[j+k] = dist(gen);
                    n3[j+k] = dist(gen);
                }
            }

            // AVX-512 operations (16 floats at once)
            for (int offset = 0; offset < AVX512_SIZE; offset += 16) {
                avx512FmaOp(n1+offset, n2+offset, n3+offset, out+offset);
            }

            // Prevent optimization
            asm volatile("" : : "r"(out) : "memory");
        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Thread " << tid << " AVX512 done in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    // Optimized AES worker with better vectorization
    static void aesWorkerOptimized(int tid) {
        pinThreadOptimal(tid);

        constexpr size_t BLOCKS = 1 << 26; // 64MB test (4x larger)
        constexpr size_t BUFFER_SIZE = BLOCKS * 16;

        // NUMA-aware allocation
        int numa_node = tid % numa_max_node();
        auto buffer = allocate_numa_buffer(BUFFER_SIZE, numa_node);
        if (!buffer) return;

        alignas(32) uint8_t keys[8][32]; // Multiple keys for parallel processing
        alignas(32) uint8_t expanded_keys[8][240];

        // Initialize multiple keys
        for (int i = 0; i < 8; ++i) {
            memset(keys[i], i + 1, 32);
            aes256KeygenOptimized(expanded_keys[i], keys[i]);
        }

        // Parallel AES operations using different keys
        const size_t chunk_size = BLOCKS / 8;
        uint8_t* buf_ptr = static_cast<uint8_t*>(buffer);

        for (int i = 0; i < 8; ++i) {
            aesVectorEncrypt(buf_ptr + i * chunk_size * 16,
                           chunk_size, expanded_keys[i]);
        }

        free_buffer(buffer, BUFFER_SIZE);
    }

    // Better thread pinning with NUMA awareness
    static void pinThreadOptimal(int core) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        // Distribute threads across NUMA nodes
        int total_cores = std::thread::hardware_concurrency();
        int actual_core = core % total_cores;

        CPU_SET(actual_core, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

        #ifdef __linux__
        // Set NUMA policy for better memory access
        int numa_node = actual_core / (total_cores / numa_max_node());
        numa_run_on_node(numa_node);
        #endif
    }

    // Optimized Collatz with better batching and SIMD hints
    static void collatzWorker(unsigned long iterations, unsigned long lower,
                             unsigned long upper, int tid,
                             std::atomic<uint64_t>& total_steps,
                             std::atomic<uint64_t>& completed_iterations) {
        pinThreadOptimal(tid);

        // Use different seeds per thread for better distribution
        pcg32 gen(static_cast<uint64_t>(tid) * 0x9e3779b9,
                  static_cast<uint64_t>(tid) * 0x85ebca6b);
        std::uniform_int_distribution<unsigned long> dist(lower, upper);

        uint64_t local_steps = 0;
        const auto start = std::chrono::high_resolution_clock::now();

        // Process in large batches for better cache utilization
        for (unsigned long i = 0; i < iterations; ) {
            const unsigned long batch = std::min(
                static_cast<unsigned long>(COLLATZ_BATCH_SIZE),
                iterations - i
            );

            uint64_t batch_steps = 0;

            // Unroll loop for better performance
            for (unsigned long j = 0; j < batch; j += 4) {
                unsigned long steps1 = 0, steps2 = 0, steps3 = 0, steps4 = 0;

                p3np1E(dist(gen), &steps1);
                p3np1E(dist(gen), &steps2);
                p3np1E(dist(gen), &steps3);
                p3np1E(dist(gen), &steps4);

                batch_steps += steps1 + steps2 + steps3 + steps4;
            }

            local_steps += batch_steps;
            i += batch;

            // Update atomics less frequently
            if (i % (COLLATZ_BATCH_SIZE * 10) == 0) {
                total_steps.fetch_add(batch_steps, std::memory_order_relaxed);
                completed_iterations.fetch_add(batch, std::memory_order_relaxed);
            }
        }

        // Final atomic updates
        total_steps.fetch_add(local_steps, std::memory_order_relaxed);
        completed_iterations.fetch_add(iterations, std::memory_order_relaxed);

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Thread " << tid << " done: " << local_steps << " steps in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    // Rest of the methods remain the same...
    std::tuple<unsigned long, unsigned long, unsigned long> getInputs(const std::string& test) {
        unsigned long its = 0, lower = 0, upper = 0;

        std::cout << test << " iterations? : ";
        if (!(std::cin >> its)) { badInput(); return {0,0,0}; }

        std::cout << "Lower limit? : ";
        if (!(std::cin >> lower)) { badInput(); return {0,0,0}; }

        std::cout << "Upper limit? : ";
        if (!(std::cin >> upper)) { badInput(); return {0,0,0}; }

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

    static void free_buffer(void* buf, size_t size) {
        #ifdef __linux__
        munmap(buf, size);
        #else
        free(buf);
        #endif
    }

    // Optimized AVX worker (existing method enhanced)
    static void avxWorker(unsigned long iterations, float lower, float upper, int tid) {
        pinThreadOptimal(tid);
        pcg32 gen(42u + tid, 54u + tid);
        std::uniform_real_distribution<float> dist(lower, upper);

        alignas(32) float n1[AVX_BUFFER_SIZE], n2[AVX_BUFFER_SIZE],
                          n3[AVX_BUFFER_SIZE], out[AVX_BUFFER_SIZE];

        const auto start = std::chrono::high_resolution_clock::now();

        for (unsigned long i = 0; i < iterations; ++i) {
            // Generate random numbers in batches
            for (int j = 0; j < AVX_BUFFER_SIZE; ++j) {
                n1[j] = dist(gen);
                n2[j] = dist(gen);
                n3[j] = dist(gen);
            }

            // Process in 8-float chunks (AVX2)
            for (int offset = 0; offset < AVX_BUFFER_SIZE; offset += 8) {
                avx(n1+offset, n2+offset, n3+offset, out+offset);
            }

            asm volatile("" : : "r"(out) : "memory");
        }

        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Thread " << tid << " AVX done in "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " ms\n";
    }

    // Placeholder functions that would need implementation in core.hpp
    static void floodL1L2Optimized(void* buffer, unsigned long iterations, size_t size) {
        // Implement optimized L1/L2 flooding with better stride patterns
    }

    static void floodMemoryOptimized(void* buffer, unsigned long iterations, size_t size) {
        // Implement optimized memory flooding with NUMA awareness
    }

    static void floodNtOptimized(void* buffer, unsigned long iterations, size_t size) {
        // Implement optimized non-temporal flooding
    }

    static void rowhammerAttackOptimized(void* buffer, unsigned long iterations, size_t size) {
        // Implement optimized rowhammer with better targeting
    }

    static void avx512FmaOp(float* a, float* b, float* c, float* out) {
        // Implement AVX-512 FMA operations
    }

    static void aes256KeygenOptimized(uint8_t* expanded_key, const uint8_t* key) {
        // Implement optimized AES key generation
    }

    static void aesVectorEncrypt(uint8_t* buffer, size_t blocks, const uint8_t* expanded_key) {
        // Implement vectorized AES encryption
    }
};

int main() {
    acts().init();
    return 0;
}