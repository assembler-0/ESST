#include <hip/hip_runtime.h>
#include <math.h>
#include <stdio.h>

__global__ void stress_kernel(float* output) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    float val = idx;

    for (int i = 0; i < 1 << 20; ++i) {
        val = sinf(val) * cosf(val) + tanf(val);
    }

    output[idx] = val;
}

extern "C" void initHIP() {
    constexpr int N = 1024 * 256; // 262144 threads

    float* d_out;
    hipMalloc(&d_out, N * sizeof(float));

    dim3 threadsPerBlock(256);
    dim3 numBlocks(N / threadsPerBlock.x);

    hipLaunchKernelGGL(stress_kernel, numBlocks, threadsPerBlock, 0, 0, d_out);
    hipDeviceSynchronize();

    // Optional: read back to verify
    float* h_out = (float*)malloc(N * sizeof(float));
    hipMemcpy(h_out, d_out, N * sizeof(float), hipMemcpyDeviceToHost);

    // Print first few results
    for (int i = 0; i < 100; ++i) {
        printf("h_out[%d] = %f\n", i, h_out[i]);
    }

    free(h_out);
    hipFree(d_out);
}
