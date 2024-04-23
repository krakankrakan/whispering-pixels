#!/bin/bash

mkdir -p build

# Build the attacker
clang++ -c attacker/Cuda.cpp -o build/Cuda.o -std=c++17 -I./include
clang++ -c attacker/Executor.cpp -o build/Executor.o -std=c++17 -I./include
clang++ build/Cuda.o build/Executor.o -o build/Executor -lcuda -lcudart -I./include

rm build/Cuda.o build/Executor.o

# Build the victim
nvcc -c victim/victim_kernel.cu -o build/victim_kernel.o --gpu-architecture sm_75 -ccbin clang-14
clang++ -c victim/Victim.cpp -o build/Victim.o -std=c++17 -I./include
clang++ build/Victim.o build/victim_kernel.o -o build/Victim -lcuda -lcudart -I./include

rm build/Victim.o build/victim_kernel.o