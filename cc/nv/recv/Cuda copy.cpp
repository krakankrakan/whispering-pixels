#include "Leak.hpp"

#include <cuda.h>
#include <cuda_runtime.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

#define THREADGROUPS        2
#define THREADGROUPSIZE     1024

#define checkCudaErrors(err) do {\
    if (err != CUDA_SUCCESS) {\
        printf("CUDA Driver API error = %04d from file <%s>, line %i.\n", err, __FILE__, __LINE__ );\
        return true;\
    }\
} while(0);

CUdevice   device;
CUcontext  context;
CUmodule   module;
CUfunction function;
size_t     totalGlobalMem;

static bool initCUDA(const char* module_file, const char* kernel_name) {
    int deviceCount = 0;
    CUresult err = cuInit(0);
    int major = 0, minor = 0;

    if (err == CUDA_SUCCESS)
        checkCudaErrors(cuDeviceGetCount(&deviceCount));

    if (deviceCount == 0) {
        printf("Error: no devices supporting CUDA\n");
        return false;
    }

    // get first CUDA device
    checkCudaErrors(cuDeviceGet(&device, 0));
    char name[100];
    cuDeviceGetName(name, 100, device);
    //printf("> Using device 0: %s\n", name);

    // get compute capabilities and the devicename
    checkCudaErrors( cuDeviceComputeCapability(&major, &minor, device) );
    //printf("> GPU Device has SM %d.%d compute capability\n", major, minor);

    checkCudaErrors( cuDeviceTotalMem(&totalGlobalMem, device) );
    //printf("  Total amount of global memory:   %llu bytes\n", (unsigned long long)totalGlobalMem);
    //printf("  64-bit Memory Address:           %s\n", (totalGlobalMem > (unsigned long long)4*1024*1024*1024L)? "YES" : "NO");

    err = cuCtxCreate(&context, 0, device);
    if (err != CUDA_SUCCESS) {
        printf("* Error initializing the CUDA context.\n");
        cuCtxDetach(context);
        return false;
    }

    err = cuModuleLoad(&module, module_file);
    if (err != CUDA_SUCCESS) {
        printf("* Error loading the module %s\n", module_file);
        cuCtxDetach(context);
        return false;
    }

    err = cuModuleGetFunction(&function, module, kernel_name);

    if (err != CUDA_SUCCESS) {
        printf("* Error getting kernel function %s\n", kernel_name);
        cuCtxDetach(context);
        return false;
    }
    return false;
}

static void finalizeCUDA() {
    cuCtxDetach(context);
}

static bool setupDeviceMemory(CUdeviceptr *d_a, CUdeviceptr *d_b, CUdeviceptr *d_c) {
    checkCudaErrors( cuMemAlloc(d_a, sizeof(int) * THREADGROUPS * THREADGROUPSIZE) );
    checkCudaErrors( cuMemAlloc(d_b, sizeof(int) * THREADGROUPS * THREADGROUPSIZE) );
    checkCudaErrors( cuMemAlloc(d_c, sizeof(int) * THREADGROUPS * THREADGROUPSIZE) );
    return false;
}

static bool releaseDeviceMemory(CUdeviceptr d_a, CUdeviceptr d_b, CUdeviceptr d_c) {
    checkCudaErrors( cuMemFree(d_a) );
    checkCudaErrors( cuMemFree(d_b) );
    checkCudaErrors( cuMemFree(d_c) );
    return false;
}

static bool runKernel(CUdeviceptr d_a, CUdeviceptr d_b, CUdeviceptr d_c) {
    void *args[3] = { &d_a, &d_b, &d_c };

    // grid for kernel: <<<N, 1>>>
    checkCudaErrors(cuLaunchKernel(function, THREADGROUPS, 1, 1,
                                    THREADGROUPSIZE, 1, 1,
                                    0, 0, args, 0));
    return false;
}

bool executeCudaKernel(const char* module_file, const char* kernel_name, bool& leaked_data) {
    //cudaDeviceReset();

    leaked_data = false;
    bool cudaError = false;

    int a[THREADGROUPS * THREADGROUPSIZE], b[THREADGROUPS * THREADGROUPSIZE], c[THREADGROUPS * THREADGROUPSIZE];
    CUdeviceptr d_a, d_b, d_c;

    // initialize host arrays
    for (int i = 0; i < THREADGROUPS * THREADGROUPSIZE; ++i) {
        a[i] = 0;
        b[i] = 0;
    }

    // initialize
    bool err = initCUDA(module_file, kernel_name);
    if (err) {
        return false;
    }

    // allocate memory
    err = setupDeviceMemory(&d_a, &d_b, &d_c);
    if (err) {
        return false;
    }

    // copy arrays to device
    //checkCudaErrors( cuMemcpyHtoD(d_a, a, sizeof(int) * THREADGROUPS * THREADGROUPSIZE) );
    //checkCudaErrors( cuMemcpyHtoD(d_b, b, sizeof(int) * THREADGROUPS * THREADGROUPSIZE) );

    unsigned int all = 0;
    unsigned int covert = 0;

    auto cc_benchmark_start_time = std::chrono::high_resolution_clock::now();

    //while(1) {
    //for (unsigned int i = 0; i < 128; i++) {
    for (unsigned int i = 0; i < 1; i++) {
        auto start_time = std::chrono::high_resolution_clock::now();

        if (runKernel(d_a, d_b, d_c)) {
            printf("runKernel error\n");
            return true;
        }

        checkCudaErrors( cuMemcpyDtoH(c, d_c, sizeof(int) * THREADGROUPS * THREADGROUPSIZE));

        for (unsigned int j = 0; j < THREADGROUPS; j++) {
            if (c[j * THREADGROUPSIZE] == 0xabcd0000) {
                covert++;
            }
            all++;
        }

        auto end_time = std::chrono::high_resolution_clock::now();

        auto exec_time = end_time - start_time;

        std::cout << "Exec time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count() << " ns" << std::endl;

        double sizeInGB = (double)(THREADGROUPS * THREADGROUPSIZE * sizeof(int)) / (1024.0 * 1024.0 * 1024.0);

        double GB_per_s = (sizeInGB) / ((unsigned long)(std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count()) / 1e9 );

        std::cout << "GB/s: " << GB_per_s << std::endl;
    }

    auto cc_benchmark_end_time = std::chrono::high_resolution_clock::now();
    auto cc_exec_time = cc_benchmark_end_time - cc_benchmark_start_time;

    double sizeInGB = (double)(covert * THREADGROUPSIZE * sizeof(int)) / (1024.0 * 1024.0 * 1024.0);

    double GB_per_s = (sizeInGB) / ((unsigned long)(std::chrono::duration_cast<std::chrono::nanoseconds>(cc_exec_time).count()) / 1e9 );

    std::cout << "Covert channel GB/s: " << GB_per_s << std::endl;

    //printf("# Kernel complete.\n");

    // copy results to host and report
    /*checkCudaErrors( cuMemcpyDtoH(c, d_c, sizeof(int) * THREADGROUPS * THREADGROUPSIZE));

    for (int i = 0; i < THREADGROUPS * THREADGROUPSIZE; ++i) {
        printf("0x%lx ", c[i]);
        if (a[i] == MAGIC_ARGUMENT || b[i] == MAGIC_ARGUMENT || c[i] == MAGIC_ARGUMENT) {
            leaked_data =  true;
        }
        if (a[i] == MAGIC_CALCULATION || b[i] == MAGIC_CALCULATION || c[i] == MAGIC_CALCULATION) {
            leaked_data =  true;
        }
    }*/


    // finish
    //printf("- Finalizing...\n");
    err = releaseDeviceMemory(d_a, d_b, d_c);
    if (err) {
        return false;
    }
    finalizeCUDA();

    return cudaError;
}