#version 320 es

layout(local_size_x = 576) in;

layout(binding = 0) readonly buffer Input0 {
    float data[];
} conv_input;

layout(binding = 1) readonly buffer Input1 {
    float data[];
} weights;

layout(binding = 3) writeonly buffer Output {
    float data[];
} conv_output;

void main() {
    uint y = gl_LocalInvocationID.x / uint(24);
    uint x = gl_LocalInvocationID.x % uint(24);

    float sum = 1.2;
    for (uint i = uint(0); i < uint(5); i ++) {
        for (uint j = uint(0); j < uint(5); j ++) {
            uint weight_idx = (gl_WorkGroupID.x * uint(5) * uint(5)) + (uint(5) * j) + i;
            uint input_idx = (uint(28) * (y + j)) + (x + i);
            float v = conv_input.data[input_idx];
        float f = weights.data[weight_idx];
        sum += f * v;
        }
    }

    conv_output.data[(uint(24) * uint(24) * gl_WorkGroupID.x) + (uint(24) * y) + x] = sum;
}