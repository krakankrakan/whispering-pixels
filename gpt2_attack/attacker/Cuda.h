#pragma once

#include <stdlib.h>

bool executeCudaKernel(const char* module_file, const char* kernel_name, bool& leaked_data);

bool patch_elf_cuda_kernel(void* patch, size_t patch_size, const char* path_in, const char* path_out);
bool patch_cuda_kernel(void* patch, size_t patch_size, const char* path_in, const char* path_out);