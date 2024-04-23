#version 320 es

layout(local_size_x = 144) in;

layout(binding = 4) readonly buffer Input0 {
    float data[];
} pool_input;

layout(binding = 5) writeonly buffer Output {
    float data[];
} pool_output;

void main() {
    uint y = gl_LocalInvocationID.x / uint(12);
    uint x = gl_LocalInvocationID.x % uint(12);

    uint mapped_x = x * uint(2);
    uint mapped_y = y * uint(2);

    float max = -100000.0;
    for (uint i = uint(0); i < uint(2); i ++) {
        for (uint j = uint(0); j < uint(2); j++) {
            uint idx = (gl_WorkGroupID.x * uint(24) * uint(24)) + (uint(24) * (mapped_y + j)) + (mapped_x + i);
            float v = pool_input.data[idx];
            if (v > max) max = v;
        }
    }

    uint out_idx = (uint(12) * uint(12) * gl_WorkGroupID.x) + (uint(12) * y) + x;
    pool_output.data[out_idx] = max;
}