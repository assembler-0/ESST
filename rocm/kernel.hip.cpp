#include <hip/hip_runtime.h>
#include <iostream>
__global__ void hello_kernel() {
    printf("Hello from the GPU!\n");
}

extern "C" void initHIP() {
    // Check if CUDA/HIP is available
    int deviceCount;
    hipError_t error = hipGetDeviceCount(&deviceCount);
    if (error != hipSuccess) {
        std::cerr << "HIP Error: " << hipGetErrorString(error) << std::endl;
        return;
    }

    if (deviceCount == 0) {
        std::cerr << "No HIP-capable devices found!" << std::endl;
        return;
    }

    std::cout << "Found " << deviceCount << " HIP device(s)" << std::endl;

    // Launch kernel
    hello_kernel<<<1, 1>>>();

    // Check for kernel launch errors
    error = hipGetLastError();
    if (error != hipSuccess) {
        std::cerr << "Kernel launch error: " << hipGetErrorString(error) << std::endl;
        return;
    }

    // Wait for kernel to complete and check for execution errors
    error = hipDeviceSynchronize();
    if (error != hipSuccess) {
        std::cerr << "Kernel execution error: " << hipGetErrorString(error) << std::endl;
        return;
    }

    std::cout << "Kernel executed successfully!" << std::endl;
}