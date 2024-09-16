#version 320 es

layout(local_size_x = 10) in;

layout(binding = 5) readonly buffer Input0 {
    float data[];
} fc_input;

layout(binding = 2) readonly buffer Input1 {
    float data[];
} weights;

layout(binding = 6) writeonly buffer Output {
    float data[];
} fc_output;

void main() {
    float inputv = 0.0;

    for (uint i = uint(0); i < uint(12); i++) {
        for (uint j = uint(0); j < uint(12); j++) {
            for (uint z = uint(0); z < uint(8); z++) {
                uint in_idx = (z * uint(12) * uint(12)) + (j * uint(12)) + i;
                uint weight_idx = (uint(12) * uint(12) * uint(8) * gl_LocalInvocationID.x) + in_idx;

                inputv += fc_input.data[in_idx] * weights.data[weight_idx];
            }
        }
    }

    float result = 1.0f / (1.0f + exp(-inputv));
    fc_output.data[gl_LocalInvocationID.x] = result;
}