#include <metal_stdlib>
using namespace metal;
/// This is a Metal Shading Language (MSL) function equivalent to the add_arrays() C function, used to perform the calculation on a GPU.

kernel void test_shader(device const int* inA,
                       device const int* inB,
                       device int* result,
                       uint index [[thread_position_in_grid]])
{
    result[index] = inA[index];
}
