#include <iostream>
#include <cuda_runtime.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <ctime>

/*
 We try to either leak the inputs/outputs of the victim kernel (abc,d,e)
 or try yo leak intermediate calculations (tmp).
 We also monitor the return values for possible corruptions.
 */

#define THREADGROUPS        64
#define THREADGROUPSIZE     1024
#define VICTIM_EXEC_COUNT   512

#include "Leak.hpp"

#define MAGIC_CALCULATION_INPUT     0x42411efd

extern void call_victimKernel(size_t threadgroups, size_t threadgroupsize, int* d_a, int* d_b, int* d_c, int* d_d, int* d_e);

int main() {
    int* a = new int[THREADGROUPS * THREADGROUPSIZE];
    int* b = new int[THREADGROUPS * THREADGROUPSIZE];
    int* c = new int[THREADGROUPS * THREADGROUPSIZE];
    int* d = new int[THREADGROUPS * THREADGROUPSIZE];
    int* e = new int[THREADGROUPS * THREADGROUPSIZE];

    std::cout << "Starting fuzzer victim" << std::endl;

    unsigned int size = THREADGROUPS * THREADGROUPSIZE * sizeof(int);
    for (int i = 0; i < THREADGROUPS * THREADGROUPSIZE; i++) {
        if (i % 16 == 0) {
            a[i] = 0xabcd0000;
        } else {
            a[i] = i;
        }
    }

    int* d_a, *d_b, *d_c, *d_d, *d_e;
    cudaMalloc((void**)&d_a, size);
    cudaMalloc((void**)&d_b, size);
    cudaMalloc((void**)&d_c, size);
    cudaMalloc((void**)&d_d, size);
    cudaMalloc((void**)&d_e, size);

    while (1) {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::cout << "[" << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << " ]: exec" << std::endl;

        cudaMemcpy(d_a, a, size, cudaMemcpyHostToDevice);

        //for (int i = 0; i < VICTIM_EXEC_COUNT; i++) {
        while (1) {
            //cudaMemcpy(d_c, c, size, cudaMemcpyHostToDevice);
            //cudaMemcpy(d_e, e, size, cudaMemcpyHostToDevice);

            call_victimKernel(THREADGROUPS, THREADGROUPSIZE, d_a, d_b, d_c, d_d, d_e);
            //cudaDeviceSynchronize();

            //cudaMemcpy(b, d_b, size, cudaMemcpyDeviceToHost);

            //cudaMemcpy(d, d_d, size, cudaMemcpyDeviceToHost);
            usleep(10000);

            /*for (int i = 0; i < THREADGROUPS * THREADGROUPSIZE; i++) {
                if ((d[i] & 0xFFFF0000) != 0xabcd0000) {
                    std::cout << "wrong: " << d[i] << std::endl;
                }
            }*/
        }
    }

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
    cudaFree(d_d);
    cudaFree(d_e);
    delete[] a;
    delete[] b;
    delete[] c;
    delete[] d;
    delete[] e;

    return 0;
}