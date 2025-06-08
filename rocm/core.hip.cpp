#include <hip/hip_runtime.h>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <thread>

// Data structure for storing benchmark results
struct BenchmarkResult {
    std::string test_name;
    double execution_time_ms;
    double operations_per_second;
    double bandwidth_gbps; // Also used for performance degradation % in thermal test
    int score;
    bool passed;
    double reference;
};

// Data structure for storing detected GPU specifications
struct GPUSpecs {
    int compute_units;
    int max_threads_per_block;
    int max_blocks_per_grid;
    size_t global_mem_size;
    size_t shared_mem_size;
    int warp_size;
    int max_registers_per_block;
    double memory_clock_mhz;
    double core_clock_mhz;
};

// MAXIMUM INTENSITY MEMORY TEST - Absolutely brutal memory patterns
__global__ void memoryTest(float* __restrict__ data,
                           float* __restrict__ data2,
                           float* __restrict__ data3,
                           float* __restrict__ data4,
                           float* __restrict__ data5,
                           float* __restrict__ data6,
                           float* __restrict__ data7,
                           float* __restrict__ data8,
                           const int size, const int iterations) {
    const unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int warp_id = tid / 32;
    const unsigned int lane_id = tid % 32;

    extern __shared__ float shared_data[];
    const unsigned int local_id = threadIdx.x;
    const unsigned int shared_size = blockDim.x;

    // Initialize shared memory with chaotic patterns
    shared_data[local_id] = data[tid % size];
    shared_data[(local_id + shared_size/2) % shared_size] = data2[tid % size];
    __syncthreads();

    float reg_bank[64]; // Maximum register usage
    for (int r = 0; r < 64; r++) {
        reg_bank[r] = data[(tid + r * 1031) % size]; // Prime number spacing
    }

    for (int i = 0; i < iterations; i++) {
        // EXTREME random access patterns - maximum cache thrashing
        const unsigned int chaos1 = (tid * 1009 + i * 1013 + warp_id * 1019) % size;
        const unsigned int chaos2 = (tid * 1021 + i * 1031 + lane_id * 1033) % size;
        const unsigned int chaos3 = (tid * 1039 + i * 1049 + blockIdx.x * 1051) % size;
        const unsigned int chaos4 = (tid * 1061 + i * 1063 + threadIdx.x * 1069) % size;
        const unsigned int chaos5 = (tid * 1087 + i * 1091 + (i*tid) * 1093) % size;
        const unsigned int chaos6 = (tid * 1097 + i * 1103 + (i^tid) * 1109) % size;
        const unsigned int chaos7 = (tid * 1117 + i * 1123 + (i+tid) * 1129) % size;
        const unsigned int chaos8 = (tid * 1151 + i * 1153 + (i*i+tid) * 1163) % size;

        // Load with maximum dependency chains
        const float val1 = data[chaos1];
        const float val2 = data2[chaos2];
        const float val3 = data3[chaos3];
        const float val4 = data4[chaos4];
        const float val5 = data5[chaos5];
        const float val6 = data6[chaos6];
        const float val7 = data7[chaos7];
        const float val8 = data8[chaos8];

        // Intensive shared memory operations with maximum contention
        for (int s = 0; s < 8; s++) {
            const unsigned int shared_idx = (local_id + s * 37 + i * 41) % shared_size;
            shared_data[shared_idx] = fmaf(shared_data[shared_idx], val1 + s, val2 * s);
            __syncthreads();
        }

        // Complex register operations with maximum ALU usage
        for (int r = 0; r < 63; r++) {
            reg_bank[r+1] = fmaf(reg_bank[r], shared_data[(local_id + r) % shared_size],
                                fmaf(val3, val4, fmaf(val5, val6, fmaf(val7, val8, reg_bank[r]))));
        }

        // Maximum memory write operations with dependencies
        data[chaos1] = fmaf(reg_bank[0], shared_data[local_id], fmaf(val1, val5, val2));
        data2[chaos2] = fmaf(reg_bank[8], shared_data[(local_id + 8) % shared_size], fmaf(val2, val6, val3));
        data3[chaos3] = fmaf(reg_bank[16], shared_data[(local_id + 16) % shared_size], fmaf(val3, val7, val4));
        data4[chaos4] = fmaf(reg_bank[24], shared_data[(local_id + 24) % shared_size], fmaf(val4, val8, val5));
        data5[chaos5] = fmaf(reg_bank[32], shared_data[(local_id + 32) % shared_size], fmaf(val5, val1, val6));
        data6[chaos6] = fmaf(reg_bank[40], shared_data[(local_id + 40) % shared_size], fmaf(val6, val2, val7));
        data7[chaos7] = fmaf(reg_bank[48], shared_data[(local_id + 48) % shared_size], fmaf(val7, val3, val8));
        data8[chaos8] = fmaf(reg_bank[56], shared_data[(local_id + 56) % shared_size], fmaf(val8, val4, val1));

        // EXTREME cache trashing loop - maximum memory system stress
        for (int j = 0; j < 32; j++) {
            const unsigned int thrash_idx1 = (tid + i * 73 + j * 79) % size;
            const unsigned int thrash_idx2 = (tid + i * 83 + j * 89) % size;
            const unsigned int thrash_idx3 = (tid + i * 97 + j * 101) % size;
            const unsigned int thrash_idx4 = (tid + i * 103 + j * 107) % size;

            data[thrash_idx1] = fmaf(data[thrash_idx1], 1.0000001f, reg_bank[j % 64]);
            data2[thrash_idx2] = fmaf(data2[thrash_idx2], 1.0000001f, reg_bank[(j+16) % 64]);
            data3[thrash_idx3] = fmaf(data3[thrash_idx3], 1.0000001f, reg_bank[(j+32) % 64]);
            data4[thrash_idx4] = fmaf(data4[thrash_idx4], 1.0000001f, reg_bank[(j+48) % 64]);
        }

        __syncthreads();
    }
}

// MAXIMUM INTENSITY COMPUTE TEST - Every possible math operation
__global__ void computationalTest(float* __restrict__ data, int size, int iterations) {
    const unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;

    float r[128]; // MAXIMUM register usage
    for (int i = 0; i < 128; i++) {
        r[i] = data[(tid + i * 127) % size]; // Prime spacing
    }

    for (int iter = 0; iter < iterations; iter++) {
        // LEVEL 1: Basic transcendental functions (maximum intensity)
        for (int i = 0; i < 127; i++) {
            r[i+1] = fmaf(sinf(r[i] * 0.1f), cosf(r[i+1] * 0.1f),
                         fmaf(tanf(r[i] * 0.05f), r[i+1], r[i]));
        }

        // LEVEL 2: Advanced transcendental functions
        for (int i = 0; i < 127; i++) {
            r[i+1] = fmaf(expf(r[i] * 0.01f), logf(fabsf(r[i+1]) + 1.0f),
                         fmaf(exp2f(r[i] * 0.01f), log2f(fabsf(r[i+1]) + 1.0f), r[i]));
        }

        // LEVEL 3: Power and root functions
        for (int i = 0; i < 127; i++) {
            r[i+1] = fmaf(powf(fabsf(r[i]) + 1.0f, 1.1f), sqrtf(fabsf(r[i+1]) + 1.0f),
                         fmaf(cbrtf(fabsf(r[i]) + 1.0f), r[i+1], r[i]));
        }

        // LEVEL 4: Hyperbolic functions
        for (int i = 0; i < 127; i++) {
            r[i+1] = fmaf(sinhf(r[i] * 0.01f), coshf(r[i+1] * 0.01f),
                         fmaf(tanhf(r[i] * 0.1f), r[i+1], r[i]));
        }

        // LEVEL 5: Inverse trigonometric functions
        for (int i = 0; i < 127; i++) {
            float clamped1 = fmaxf(-0.999f, fminf(0.999f, r[i] * 0.001f));
            float clamped2 = fmaxf(-0.999f, fminf(0.999f, r[i+1] * 0.001f));
            r[i+1] = fmaf(asinf(clamped1), acosf(clamped2),
                         fmaf(atanf(r[i]), r[i+1], r[i]));
        }

        // LEVEL 6: Special functions and complex operations
        for (int i = 0; i < 127; i++) {
            r[i+1] = fmaf(erfcf(r[i] * 0.1f), lgammaf(fabsf(r[i+1]) + 2.0f),
                         fmaf(j0f(r[i]), j1f(r[i+1]), r[i]));
        }

        // LEVEL 7: Maximum complexity cascade
        for (int i = 0; i < 127; i++) {
            r[i+1] = fmaf(sinf(expf(r[i] * 0.001f)), cosf(logf(fabsf(r[i+1]) + 1.0f)),
                         fmaf(tanf(sqrtf(fabsf(r[i]) + 1.0f)), powf(fabsf(r[i+1]) + 1.0f, 0.9f),
                              fmaf(sinhf(r[i] * 0.01f), coshf(r[i+1] * 0.01f), r[i])));
        }

        // LEVEL 8: Ultra-complex dependency chains
        for (int i = 0; i < 127; i++) {
            float temp1 = fmaf(r[i], r[(i+32) % 128], r[(i+64) % 128]);
            float temp2 = fmaf(r[i+1], r[(i+33) % 128], r[(i+65) % 128]);
            r[i+1] = fmaf(sinf(temp1), cosf(temp2), fmaf(expf(temp1 * 0.001f), logf(fabsf(temp2) + 1.0f), r[i]));
        }

        // LEVEL 9: Maximum register interdependency
        for (int pass = 0; pass < 4; pass++) {
            for (int i = 0; i < 128; i++) {
                r[i] = fmaf(r[i], r[(i + 1) % 128],
                           fmaf(r[(i + 2) % 128], r[(i + 3) % 128],
                                fmaf(r[(i + 4) % 128], r[(i + 5) % 128], r[(i + 6) % 128])));
            }
        }
    }

    // Final reduction with maximum ALU usage
    float final_result = 0.0f;
    for (int i = 0; i < 128; i++) {
        final_result = fmaf(final_result, r[i], sinf(r[i] * 0.01f));
    }
    data[tid % size] = final_result;
}

// MAXIMUM INTENSITY ATOMIC TEST - Maximum contention and operations
__global__ void atomicTest(int* __restrict__ counters,
                          float* __restrict__ float_data,
                          unsigned long long* __restrict__ ull_data,
                          double* __restrict__ double_data,
                          const int iterations) {
    const unsigned int tid = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int warp_id = tid / 32;
    const unsigned int lane_id = tid % 32;

    for (int i = 0; i < iterations; i++) {
        // MAXIMUM contention - all threads fight for same locations
        const unsigned int ultra_contested_int = i % 16;        // Only 16 locations!
        const unsigned int ultra_contested_float = i % 32;      // Only 32 locations!
        const unsigned int ultra_contested_ull = i % 64;        // Only 64 locations!
        const unsigned int ultra_contested_double = i % 128;    // Only 128 locations!

        // ROUND 1: Integer atomics (10 operations)
        atomicAdd(&counters[ultra_contested_int], 1);
        atomicSub(&counters[ultra_contested_int], 1);
        atomicMax(&counters[ultra_contested_int], tid);
        atomicMin(&counters[ultra_contested_int], tid);
        atomicExch(&counters[ultra_contested_int], tid + i);
        atomicOr(&counters[ultra_contested_int], tid);
        atomicAnd(&counters[ultra_contested_int], ~tid);
        atomicXor(&counters[ultra_contested_int], tid);
        atomicCAS(&counters[ultra_contested_int], tid, tid + 1);
        atomicInc((unsigned int*)&counters[ultra_contested_int], 1000000);

        // ROUND 2: Float atomics (5 operations)
        atomicAdd(&float_data[ultra_contested_float], 1.0f);
        atomicAdd(&float_data[ultra_contested_float], -1.0f);
        atomicExch(&float_data[ultra_contested_float], (float)tid);
        atomicMax(&float_data[ultra_contested_float], (float)tid);
        atomicMin(&float_data[ultra_contested_float], (float)tid);

        // ROUND 3: ULL atomics (8 operations)
        atomicAdd(&ull_data[ultra_contested_ull], 1ULL);
        atomicSub(&ull_data[ultra_contested_ull], 1ULL);
        atomicMax(&ull_data[ultra_contested_ull], (unsigned long long)tid);
        atomicMin(&ull_data[ultra_contested_ull], (unsigned long long)tid);
        atomicExch(&ull_data[ultra_contested_ull], (unsigned long long)(tid + i));
        atomicOr(&ull_data[ultra_contested_ull], (unsigned long long)tid);
        atomicAnd(&ull_data[ultra_contested_ull], ~(unsigned long long)tid);
        atomicXor(&ull_data[ultra_contested_ull], (unsigned long long)tid);

        // ROUND 4: Double atomics (2 operations)
        atomicAdd(&double_data[ultra_contested_double], 1.0);
        atomicAdd(&double_data[ultra_contested_double], -1.0);

        // ROUND 5: INSANE contention - single location battles
        const int insane_location = 0;
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);
        atomicAdd(&counters[insane_location], 1);

        // ROUND 6: Warp-level mega contention
        const int warp_contested = warp_id % 4;
        for (int w = 0; w < 5; w++) {
            atomicAdd(&counters[warp_contested], 1);
        }
    }
}

class GPUstress {
private:
    std::vector<BenchmarkResult> results;
    GPUSpecs gpu_specs;

    // Updated reference values for extreme tests
    const double REF_MEMORY_OPS = 5.8e11;   // Much higher for 8-buffer test
    const double REF_COMPUTE_OPS = 8.7e13;  // Much higher for 128-register test
    const double REF_ATOMIC_OPS = 1.2e11;   // Much higher for 40-op test
    const double REF_BANDWIDTH = 2016;      // Higher bandwidth expectation

public:
    void detect_gpu_specifications() {
        hipDeviceProp_t prop;
        hipGetDeviceProperties(&prop, 0);

        gpu_specs.compute_units = prop.multiProcessorCount;
        gpu_specs.max_threads_per_block = prop.maxThreadsPerBlock;
        gpu_specs.max_blocks_per_grid = prop.maxGridSize[0];
        gpu_specs.global_mem_size = prop.totalGlobalMem;
        gpu_specs.shared_mem_size = prop.sharedMemPerBlock;
        gpu_specs.warp_size = prop.warpSize;
        gpu_specs.max_registers_per_block = prop.regsPerBlock;
        gpu_specs.memory_clock_mhz = prop.memoryClockRate / 1000.0;
        gpu_specs.core_clock_mhz = prop.clockRate / 1000.0;

        std::cout << "=== GPU SPECIFICATIONS DETECTED ===\n";
        std::cout << "GPU: " << prop.name << "\n";
        std::cout << "Compute Units: " << gpu_specs.compute_units << "\n";
        std::cout << "Global Memory: " << gpu_specs.global_mem_size / (1024*1024*1024) << " GB\n";
        std::cout << "Shared Memory/Block: " << gpu_specs.shared_mem_size / 1024 << " KB\n";
        std::cout << "Core Clock: " << gpu_specs.core_clock_mhz << " MHz\n\n";
    }

    // EXTREME MEMORY TEST - 8 buffers, maximum operations
    [[nodiscard]] BenchmarkResult initMemoryTest(const int iterations) const {
        constexpr int OPS_PER_THREAD_PER_ITER = 328; // 8 loads + 64 reg ops + 8 shared loops*3 + 8 writes + 32 thrash*4 + misc
        const size_t size = std::min(static_cast<size_t>(128 * 1024 * 1024), gpu_specs.global_mem_size / 32);

        float *d_data1, *d_data2, *d_data3, *d_data4, *d_data5, *d_data6, *d_data7, *d_data8;
        hipMalloc(&d_data1, size * sizeof(float)); hipMalloc(&d_data2, size * sizeof(float));
        hipMalloc(&d_data3, size * sizeof(float)); hipMalloc(&d_data4, size * sizeof(float));
        hipMalloc(&d_data5, size * sizeof(float)); hipMalloc(&d_data6, size * sizeof(float));
        hipMalloc(&d_data7, size * sizeof(float)); hipMalloc(&d_data8, size * sizeof(float));

        std::vector<float> h_data(size);
        std::generate(h_data.begin(), h_data.end(), []() { return static_cast<float>(std::rand()) / RAND_MAX; });
        hipMemcpy(d_data1, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data2, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data3, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data4, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data5, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data6, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data7, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data8, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);

        // Maximum block size with shared memory considerations
        int block_size = std::min((int)gpu_specs.max_threads_per_block,
                                 static_cast<int>(gpu_specs.shared_mem_size / (2 * sizeof(float))));
        block_size = (block_size / gpu_specs.warp_size) * gpu_specs.warp_size;
        if (block_size == 0) block_size = gpu_specs.warp_size;

        dim3 block(block_size);
        dim3 grid(std::min(gpu_specs.max_blocks_per_grid, gpu_specs.compute_units * 16)); // More blocks
        size_t shared_mem_size = block.x * 2 * sizeof(float); // Double shared memory usage

        const auto start = std::chrono::high_resolution_clock::now();
        memoryTest<<<grid, block, shared_mem_size>>>(d_data1, d_data2, d_data3, d_data4,
                                                     d_data5, d_data6, d_data7, d_data8, size, iterations);
        hipDeviceSynchronize();
        const auto end = std::chrono::high_resolution_clock::now();

        const double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        const double total_ops = static_cast<double>(grid.x) * block.x * iterations * OPS_PER_THREAD_PER_ITER;
        const double ops_per_second = total_ops / (time_ms / 1000.0);
        const double bandwidth_gbps = (total_ops * sizeof(float) / time_ms) / 1e6;
        const int score = std::min(1000, static_cast<int>(1000 * ops_per_second / REF_MEMORY_OPS));

        hipFree(d_data1); hipFree(d_data2); hipFree(d_data3); hipFree(d_data4);
        hipFree(d_data5); hipFree(d_data6); hipFree(d_data7); hipFree(d_data8);
        return {"Memory", time_ms, ops_per_second, bandwidth_gbps, score, score > 100, (bandwidth_gbps / REF_BANDWIDTH) * 100.0};
    }

    // EXTREME COMPUTATIONAL TEST - 128 registers, maximum math operations
    [[nodiscard]] BenchmarkResult initComputationalTest(const int iterations) const {
        constexpr int OPS_PER_THREAD_PER_ITER = 89727; // 127*9 levels + 128*4 interdep + reduction ops
        const size_t size = 2 * 1024 * 1024; // Larger for 128 registers
        float* d_data;
        hipMalloc(&d_data, size * sizeof(float));
        std::vector<float> h_data(size);
        std::generate(h_data.begin(), h_data.end(), [](){ return static_cast<float>(rand()) / RAND_MAX * 10.f; });
        hipMemcpy(d_data, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);

        // Reduced block size due to extreme register usage
        dim3 block(std::min(256, (int)gpu_specs.max_threads_per_block));
        dim3 grid(std::min(gpu_specs.max_blocks_per_grid, gpu_specs.compute_units * 16));

        const auto start = std::chrono::high_resolution_clock::now();
        computationalTest<<<grid, block>>>(d_data, size, iterations);
        hipDeviceSynchronize();
        const auto end = std::chrono::high_resolution_clock::now();

        const double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        const double total_ops = static_cast<double>(grid.x) * block.x * iterations * OPS_PER_THREAD_PER_ITER;
        const double ops_per_second = total_ops / (time_ms / 1000.0);
        const int score = std::min(1000, static_cast<int>(1000 * ops_per_second / REF_COMPUTE_OPS));

        hipFree(d_data);
        return {"Computational", time_ms, ops_per_second, 0.0, score, score > 200, (ops_per_second / REF_COMPUTE_OPS) * 100.0};
    }

    // EXTREME ATOMIC TEST - 40 operations per thread per iteration
    [[nodiscard]] BenchmarkResult initAtomicTest(const int iterations) const {
        constexpr int ATOMIC_OPS_PER_THREAD_PER_ITER = 40; // 10+5+8+2+10+5 = 40 atomic ops

        int num_counters = 2048;

        int* d_counters;
        float* d_float_data;
        unsigned long long* d_ull_data;
        double* d_double_data;

        hipMalloc(&d_counters, num_counters * sizeof(int));
        hipMalloc(&d_float_data, num_counters * sizeof(float));
        hipMalloc(&d_ull_data, num_counters * sizeof(unsigned long long));
        hipMalloc(&d_double_data, num_counters * sizeof(double));

        hipMemset(d_counters, 0, num_counters * sizeof(int));
        hipMemset(d_float_data, 0, num_counters * sizeof(float));
        hipMemset(d_ull_data, 0, num_counters * sizeof(unsigned long long));
        hipMemset(d_double_data, 0, num_counters * sizeof(double));

        // Maximum possible grid size for atomic contention
        const int max_blocks = std::min(gpu_specs.compute_units * 16, 2048);
        dim3 block(gpu_specs.max_threads_per_block);
        dim3 grid(max_blocks);

        const auto start = std::chrono::high_resolution_clock::now();
        atomicTest<<<grid, block>>>(d_counters, d_float_data, d_ull_data, d_double_data, iterations);
        hipDeviceSynchronize();
        const auto end = std::chrono::high_resolution_clock::now();

        const double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        const double total_atomic_ops = static_cast<double>(grid.x) * block.x * iterations * ATOMIC_OPS_PER_THREAD_PER_ITER;
        const double atomic_ops_per_second = total_atomic_ops / (time_ms / 1000.0);

        const int score = std::min(1000, static_cast<int>(1000 * atomic_ops_per_second / REF_ATOMIC_OPS));

        hipFree(d_counters);
        hipFree(d_float_data);
        hipFree(d_ull_data);
        hipFree(d_double_data);

        return {"Atomic", time_ms, atomic_ops_per_second, 0.0, score, score > 200,
                (atomic_ops_per_second / REF_ATOMIC_OPS) * 100.0};
    }

    void init(const int iterations) {
        std::cout << "=== MAXIMUM INTENSITY GPU STRESS TEST v3.0 ===\n";
        std::cout << "EXTREME WARNING: This test pushes hardware to absolute limits!\n";
        std::cout << "Monitor temperatures, power consumption, and stability!\n";
        std::cout << "May cause system instability or hardware damage!\n\n";

        detect_gpu_specifications();

        results.push_back(initMemoryTest(iterations));
        results.push_back(initComputationalTest(iterations));
        results.push_back(initAtomicTest(iterations));
        printDetailedResults();
        calculateScore();
    }

    void printDetailedResults() const {
        std::cout << "\n=== TEST RESULTS ===\n";
        std::cout << std::left << std::setw(20) << "Test category"
                  << std::right << std::setw(12) << "Time (ms)"
                  << std::setw(13) << "Ops/sec"
                  << std::setw(10) << "BW"
                  << std::setw(8) << "Score"
                  << std::setw(10) << "REF %" << "\n";
        std::cout << std::string(73, '=') << "\n";

        for (const auto& r : results) {
            std::cout << std::left << std::setw(20) << r.test_name
                      << std::right << std::setw(12) << std::fixed << std::setprecision(1) << r.execution_time_ms
                      << std::setw(13) << std::scientific << std::setprecision(2) << r.operations_per_second
                      << std::setw(10) << std::fixed << std::setprecision(1) << r.bandwidth_gbps
                      << std::setw(8) << r.score
                      << std::setw(10) << std::setprecision(1) << r.reference
                      << "\n";
        }
        std::cout << "\n";
    }

    void calculateScore() const {
        if (results.empty()) return;

        double weighted_score = 0.0;
        const std::vector<double> weights = {0.4, 0.4, 0.2};

        for (size_t i = 0; i < results.size(); i++) {
            weighted_score += weights[i] * results[i].score;
        }

        std::cout << "=== FINAL SCORE ===\n";
        std::cout << "Score: " << static_cast<int>(weighted_score) << "/500\n";
    }
};

// External entry point
extern "C" void initGPU(const int iterations) {
    GPUstress tester;
    tester.init(iterations);
    std::cout << "\n=== MAXIMUM INTENSITY TEST COMPLETE ===\n";
    std::cout << "If system is unstable, reduce iterations or check cooling!\n";
}