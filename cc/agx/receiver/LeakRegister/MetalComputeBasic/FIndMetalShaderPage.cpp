#include "FIndMetalShaderPage.hpp"

#include <iomanip>
#include <ctime>
#include <chrono>

#include <iostream>

std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

double maxGBperS;

extern "C" {
    extern unsigned int arrayLength;

    void setStartTime() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    void setEndTime() {
        end_time = std::chrono::high_resolution_clock::now();
    }

    unsigned long getExecutionTimeInNS() {
        auto exec_time = end_time - start_time;
        
        return (unsigned long)(std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count());
    }

    double calculateGBperS(unsigned int* ptr) {
        unsigned int covert = 0;
        double GBperS = 0;
        
        size_t threadgroups = arrayLength / 1024;
        
        bool cc_seen [1024 * 1024];
        for (unsigned int j = 0; j < arrayLength; j++) {
            cc_seen[j] = false;
        }

        for (unsigned int j = 0; j < arrayLength; j+=16) {
            if (ptr[j] == 0xab12cd34) {
                for (unsigned int k = 1; k < 16; k++) {
                    if (ptr[j + k] < 1024 * 1024)
                        cc_seen[ptr[j + k]] = true;
                }
            }
        }

        for (unsigned int j = 0; j < arrayLength; j++) {
            if (cc_seen[j]) {
                covert++;
            }
        }

        auto exec_time = end_time - start_time;
        double covertSizeInGB = (double)(covert * sizeof(int)) / (1024.0 * 1024.0 * 1024.0);
        GBperS = (covertSizeInGB) / ((unsigned long)(std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count()) / 1e9 );
        
        std::cout << "Covert channel GB/s: " << GBperS << " | max: " << maxGBperS << std::endl;
        
        covertSizeInGB = (double)(arrayLength * sizeof(int)) / (1024.0 * 1024.0 * 1024.0);
        GBperS = (covertSizeInGB) / ((unsigned long)(std::chrono::duration_cast<std::chrono::nanoseconds>(exec_time).count()) / 1e9 );
        std::cout << "Raw leakage GB/s: " << GBperS << std::endl;
        
        if (GBperS > maxGBperS) {
            maxGBperS = GBperS;
        }
        
        return GBperS;
    }
}
