__global__ void victimKernel(int* a, int* b, int* c, int *d, int *e) {
    d[blockIdx.x * blockDim.x + threadIdx.x] = a[blockIdx.x * blockDim.x + threadIdx.x];
}

void call_victimKernel(size_t threadgroups, size_t threadgroupsize, int* d_a, int* d_b, int* d_c, int* d_d, int* d_e) {
    victimKernel<<<threadgroups, threadgroupsize>>>(d_a, d_b, d_c, d_d, d_e);
}