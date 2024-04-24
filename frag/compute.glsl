#version 310 es

layout(local_size_x = 256) in;

layout(binding = 0) readonly buffer Input0 {
    uint data[];
} input0;

layout(binding = 1) readonly buffer Input1 {
    uint data[];
} input1;

layout(binding = 2) writeonly buffer Output {
    uint data[];
} output0;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    uint f = input0.data[idx] + input1.data[idx];
    output0.data[idx] = uint(0x12ab34cd) + f;
}