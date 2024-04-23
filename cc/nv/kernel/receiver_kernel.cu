extern "C" {
    __global__ void leaker_kernel(unsigned int *a, unsigned int *b, unsigned int *c) {
        c[blockIdx.x * 1024 + threadIdx.x] = 0x12ab34cd;
    }
}