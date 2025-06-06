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

__global__ void memoryTest(float* __restrict__ data,
                           float* __restrict__ data2,
                           float* __restrict__ data3,
                           float* __restrict__ data4,
                           const int size, const int iterations) {
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;

    extern __shared__ float shared_data[];
    const int local_id = threadIdx.x;

    shared_data[local_id] = data[tid % size];
    __syncthreads();

    for (int i = 0; i < iterations; i++) {
        // More complex and unpredictable address patterns
        int pattern1 = (tid * 31 + i * 17) % size;
        int pattern2 = (tid * 41 + i * 23) % size;
        int pattern3 = (tid * 53 + i * 29) % size;
        int pattern4 = (tid * 61 + i * 37) % size;

        float val1 = data[pattern1];
        float val2 = data2[pattern2];
        float val3 = data3[pattern3];
        float val4 = data4[pattern4];

        // Increased computational dependency and shared memory use
        float shared_val = shared_data[(local_id + i) % blockDim.x];
        float result = fmaf(val1, val2, shared_val);
        result = fmaf(result, val3, val4);

        shared_data[local_id] = result;
        __syncthreads();

        // Write back results with dependencies on other threads' work
        data[pattern1] = fmaf(result, shared_data[(local_id + 17) % blockDim.x], val3);
        data2[pattern2] = fmaf(val2, shared_data[(local_id + 31) % blockDim.x], val4);
        data3[pattern3] = fmaf(val3, shared_data[(local_id + 47) % blockDim.x], val1);
        data4[pattern4] = fmaf(val4, shared_data[(local_id + 61) % blockDim.x], val2);

        // More intense cache-trashing loop
        for (int j = 0; j < 12; j++) {
            int chaos_idx = (tid + i * 13 + j * 19) % size;
            data[chaos_idx] = fmaf(data[chaos_idx], 1.000001f, 0.000001f);
        }
        __syncthreads();
    }
}

// Stresses ALUs with data-dependent branching and heavier, more complex math
__global__ void computationalTest(float* __restrict__ data, int size, int iterations) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    float r[32]; // Maximize register usage
    for (int i = 0; i < 32; i++) {
        r[i] = data[(tid + i * 32) % size];
    }

    for (int iter = 0; iter < iterations; iter++) {
            for (int i = 0; i < 31; i++) r[i+1] = fmaf(sinf(r[i]), cosf(r[i+1] * 1.1f), tanf(r[i] * 0.9f));
            for (int i = 0; i < 31; i++) r[i+1] = fmaf(expf(r[i] * 0.1f), logf(fabsf(r[i+1]) + 1.0f), r[i]);
            for (int i = 0; i < 31; i++) r[i+1] = fmaf(powf(fabsf(r[i]) + 1.0f, 1.2f), sqrtf(fabsf(r[i+1]) + 1.0f), r[i]);
            for (int i = 0; i < 31; i++) r[i+1] = sinhf(r[i] * 0.1f) + coshf(r[i+1] * 0.1f) + tanhf(r[i]);
            for (int i = 0; i < 31; i++) {
                float val = fmaxf(-0.99f, fminf(0.99f, r[i] * 0.01f));
                r[i+1] = asinf(val) + acosf(val) + atanf(r[i+1]);
            }
            for (int i = 0; i < 31; i++) {
                r[i+1] = fmaf(r[i], sinf(r[i+1]), fmaf(r[i+1], cosf(r[i]), expf(r[i] * 0.01f)));
            }

        for (int i = 0; i < 32; i++) {
            r[i] = fmaf(r[i], r[(i + 3) % 32], r[(i + 7) % 32]);
        }
    }

    float final_result = 0.0f;
    for (int i = 0; i < 32; i++) final_result += r[i];
    data[tid % size] = final_result;
}

// Stresses atomic units with higher contention and more operations
__global__ void atomicTest(int* __restrict__ counters,
                               float* __restrict__ float_data,
                               unsigned long long* __restrict__ ull_data,
                               int iterations) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int warp_id = tid / 32;

    for (int i = 0; i < iterations; i++) {
        // High contention - many threads target same locations
        int contested_int = (warp_id + i) % 64;          // Only 64 locations for int atomics
        int contested_float = (tid + i) % 128;           // 128 locations for float atomics
        int contested_ull = (tid * 2 + i) % 256;         // 256 locations for ull atomics

        // Pure atomic operations - exactly 15 per thread per iteration
        atomicAdd(&counters[contested_int], 1);                    // 1
        atomicSub(&counters[contested_int], 1);                    // 2
        atomicMax(&counters[contested_int], tid);                  // 3
        atomicMin(&counters[contested_int], tid);                  // 4
        atomicExch(&counters[contested_int], tid + i);             // 5

        atomicAdd(&float_data[contested_float], 1.0f);             // 6
        atomicExch(&float_data[contested_float], 2.5f);            // 7

        atomicAdd(&ull_data[contested_ull], 1ULL);                 // 8
        atomicMax(&ull_data[contested_ull], (unsigned long long)tid); // 9
        atomicExch(&ull_data[contested_ull], (unsigned long long)(tid + i)); // 10

        // Ultra-high contention - all threads in warp hit same location
        int ultra_contested = i % 8;
        atomicAdd(&counters[ultra_contested], 1);                  // 11
        atomicAdd(&counters[ultra_contested], 1);                  // 12
        atomicAdd(&counters[ultra_contested], 1);                  // 13
        atomicAdd(&counters[ultra_contested], 1);                  // 14
        atomicAdd(&counters[ultra_contested], 1);                  // 15
    }
}


class GPUstress {
private:
    std::vector<BenchmarkResult> results;
    GPUSpecs gpu_specs;

    // UPPED a lot, Reference scores adjusted for high-end GPUs (e.g., RTX 4090 / RX 7900 XTX)
    const double REF_MEMORY_OPS = 2.3e11;
    const double REF_COMPUTE_OPS = 1.1e13;
    const double REF_ATOMIC_OPS = 3.4e10;
    const double REF_BANDWIDTH = 1008;

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

    // UPDATED with better op counting
    BenchmarkResult initMemoryTest(const int iterations) const {
        constexpr int OPS_PER_THREAD_PER_ITER = 36;
        const size_t size = std::min(static_cast<size_t>(64 * 1024 * 1024), gpu_specs.global_mem_size / 16);

        float *d_data1, *d_data2, *d_data3, *d_data4;
        hipMalloc(&d_data1, size * sizeof(float)); hipMalloc(&d_data2, size * sizeof(float));
        hipMalloc(&d_data3, size * sizeof(float)); hipMalloc(&d_data4, size * sizeof(float));

        std::vector<float> h_data(size);
        std::generate(h_data.begin(), h_data.end(), []() { return static_cast<float>(std::rand()) / RAND_MAX; });
        hipMemcpy(d_data1, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data2, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data3, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);
        hipMemcpy(d_data4, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);

        int block_size = std::min((int)gpu_specs.max_threads_per_block, static_cast<int>(gpu_specs.shared_mem_size / sizeof(float)));
        block_size = (block_size / gpu_specs.warp_size) * gpu_specs.warp_size;
        if (block_size == 0) block_size = gpu_specs.warp_size;

        dim3 block(block_size);
        dim3 grid(std::min(gpu_specs.max_blocks_per_grid, gpu_specs.compute_units * 8));
        size_t shared_mem_size = block.x * sizeof(float);

        const auto start = std::chrono::high_resolution_clock::now();
        memoryTest<<<grid, block, shared_mem_size>>>(d_data1, d_data2, d_data3, d_data4, size, iterations);
        hipDeviceSynchronize();
        const auto end = std::chrono::high_resolution_clock::now();

        const double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        const double total_ops = static_cast<double>(grid.x) * block.x * iterations * OPS_PER_THREAD_PER_ITER;
        const double ops_per_second = total_ops / (time_ms / 1000.0);
        const double bandwidth_gbps = (total_ops * sizeof(float) / time_ms) / 1e6;
        const int score = std::min(1000, static_cast<int>(1000 * ops_per_second / REF_MEMORY_OPS));

        hipFree(d_data1); hipFree(d_data2); hipFree(d_data3); hipFree(d_data4);
        return {"Memory", time_ms, ops_per_second, bandwidth_gbps, score, score > 100, (bandwidth_gbps / REF_BANDWIDTH) * 100.0};
    }

    // UPDATED with better op counting
    BenchmarkResult initComputationalTest(const int iterations) const {
        constexpr int OPS_PER_THREAD_PER_ITER = 13332;
        const size_t size = 1024 * 1024;
        float* d_data;
        hipMalloc(&d_data, size * sizeof(float));
        std::vector<float> h_data(size);
        std::generate(h_data.begin(), h_data.end(), [](){ return static_cast<float>(rand()) / RAND_MAX * 10.f; });
        hipMemcpy(d_data, h_data.data(), size * sizeof(float), hipMemcpyHostToDevice);

        dim3 block(gpu_specs.max_threads_per_block);
        dim3 grid(std::min(gpu_specs.max_blocks_per_grid, gpu_specs.compute_units * 8));

        const auto start = std::chrono::high_resolution_clock::now();
        computationalTest<<<grid, block>>>(d_data, size, iterations);
        hipDeviceSynchronize();
        const auto end = std::chrono::high_resolution_clock::now();

        const double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        const double total_ops = static_cast<double>(grid.x) * block.x * iterations * OPS_PER_THREAD_PER_ITER; // Recalculated op count
        const double ops_per_second = total_ops / (time_ms / 1000.0);
        const int score = std::min(1000, static_cast<int>(1000 * ops_per_second / REF_COMPUTE_OPS));

        hipFree(d_data);
        return {"Computational", time_ms, ops_per_second, 0.0, score, score > 200, (ops_per_second / REF_COMPUTE_OPS) * 100.0};
    }

    // UPDATED with better op counting
    BenchmarkResult initAtomicTest(int iterations) {
        constexpr int ATOMIC_OPS_PER_THREAD_PER_ITER = 15; // Exactly 15 atomic ops

        // Smaller arrays since we're creating high contention
        int num_counters = 1024;  // Reduced for more contention

        int* d_counters;
        float* d_float_data;
        unsigned long long* d_ull_data;

        hipMalloc(&d_counters, num_counters * sizeof(int));
        hipMalloc(&d_float_data, num_counters * sizeof(float));
        hipMalloc(&d_ull_data, num_counters * sizeof(unsigned long long));

        hipMemset(d_counters, 0, num_counters * sizeof(int));
        hipMemset(d_float_data, 0, num_counters * sizeof(float));
        hipMemset(d_ull_data, 0, num_counters * sizeof(unsigned long long));

        // Reasonable grid size
        int reasonable_blocks = std::min(gpu_specs.compute_units * 8, 1024);
        dim3 block(gpu_specs.max_threads_per_block);
        dim3 grid(reasonable_blocks);

        const auto start = std::chrono::high_resolution_clock::now();
        atomicTest<<<grid, block>>>(d_counters, d_float_data, d_ull_data, iterations);
        hipDeviceSynchronize();
        const auto end = std::chrono::high_resolution_clock::now();

        const double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        const double total_atomic_ops = static_cast<double>(grid.x) * block.x * iterations * ATOMIC_OPS_PER_THREAD_PER_ITER;
        const double atomic_ops_per_second = total_atomic_ops / (time_ms / 1000.0);

        const int score = std::min(1000, static_cast<int>(1000 * atomic_ops_per_second / REF_ATOMIC_OPS));

        hipFree(d_counters);
        hipFree(d_float_data);
        hipFree(d_ull_data);

        return {"Atomic", time_ms, atomic_ops_per_second, 0.0, score, score > 200,
                (atomic_ops_per_second / REF_ATOMIC_OPS) * 100.0};
    }

    // UPDATED to use the much heavier chaos test
    void init(const int iterations) {
        std::cout << "=== EXTREME GPU STRESS TEST v2.0 ===\n";
        std::cout << "WARNING: This revised test is significantly more demanding.\n";
        std::cout << "Monitor temperatures and system stability carefully.\n\n";

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

        std::cout << "=== FINAL VERDICT ===\n";
        std::cout << "Score: " << static_cast<int>(weighted_score) << "/1000\n";
    }
};

// External entry point
extern "C" void initGPU(const int iterations) {
    GPUstress tester;
    tester.init(iterations);
    std::cout << "\n=== TEST COMPLETE ===\n";
}