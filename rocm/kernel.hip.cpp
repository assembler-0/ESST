#include <hip/hip_runtime.h>
#include "stress.h"

__global__ void hello_kernel() {
    printf("Hello from the GPU!\n");
}
#define HIP_CHECK(call) \
do { \
hipError_t err = call; \
if (err != hipSuccess) { \
fprintf(stderr, "HIP error: %s at %s:%d\n", hipGetErrorString(err), __FILE__, __LINE__); \
exit(EXIT_FAILURE); \
} \
} while (0)

extern "C" void init() {
    hello_kernel<<<1, 1>>>();
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipDeviceSynchronize());
}