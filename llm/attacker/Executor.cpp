#include "Cuda.h"

#include <iostream>

/*
Kernel source code:
extern "C" {
    __global__ void leaker_kernel(unsigned int *a, unsigned int *b, unsigned int *c) {
        int out_idx = threadIdx.x + blockIdx.x * 768;
        c[out_idx] = 0x12ab34cd;
    }
}

modified last instruction to return uninitialized register
*/
int main() {
    bool cudaError = true;
    bool leaked_data = false;

    cudaError = executeCudaKernel("attacker_kernels/test2", "leaker_kernel", leaked_data);

    return 0;
}