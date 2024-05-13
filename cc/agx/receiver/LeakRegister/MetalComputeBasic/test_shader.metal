#include <metal_stdlib>
using namespace metal;

kernel void test_shader(device const int* inA,
                       device const int* inB,
                       device int* result,
                       uint index [[thread_position_in_grid]])
{
    result[index] = 0xab12cd34;
}
