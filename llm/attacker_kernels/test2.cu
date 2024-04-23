extern "C" {
    __global__ void leaker_kernel(unsigned int *a, unsigned int *b, unsigned int *c) {
        int out_idx = threadIdx.x + blockIdx.x * 768;
        c[out_idx] = 0x12ab34cd;
    }
}