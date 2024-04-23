#include "Cuda.h"

#include <iostream>

int main() {
    bool cudaError = true;
    bool leaked_data = false;

    //std::cout << "Cuda kernel executor" << std::endl;

    cudaError = executeCudaKernel("leak_kernel", "leaker_kernel", leaked_data);

    //std::cout << "err: " << std::to_string(cudaError) << std::endl;
    if (leaked_data) {
        std::cout << "successfully leaked magic value!" << std::endl;
    } else {
        std::cout << "did not see magic value" << std::endl;
    }

    return 0;
}