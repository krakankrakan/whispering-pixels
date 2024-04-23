#version 320 es

layout(local_size_x = 64) in;

layout(binding = 0) readonly buffer Input0 {
    float data[];
} weights;

layout(binding = 1) readonly buffer Input1 {
    float data[];
} previous_layer_out;

layout(binding = 2) writeonly buffer Output {
    float data[];
} output0;

float sigmoid(float x) {
    return 1.0 / (1.0 + exp(-x));
}

void main() {
    float t = 0.0f;
    for (uint i = uint(0); i < uint(64); i++) {
        t += weights.data[gl_GlobalInvocationID.x * uint(64) + i] * previous_layer_out.data[i];
    }

    t += 0.1;

    output0.data[gl_GlobalInvocationID.x] = sigmoid(t);
}