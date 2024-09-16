#version 320 es

layout(local_size_x = 192) in;

layout(binding = 3) readonly buffer Input0 {
    float data[];
} relu_input;

layout(binding = 4) writeonly buffer Output {
    float data[];
} relu_output;

void main() {
    uint col = gl_GlobalInvocationID.x;

    float v = relu_input.data[col];
    if (v < 0.0) v = 0.0;
    relu_output.data[col] = v;
}