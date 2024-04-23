#!/bin/bash

nvcc receiver_kernel.cu -o receiver_kernel --cubin -ccbin clang-14 --gpu-architecture sm_75