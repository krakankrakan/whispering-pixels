#version 320 es

layout(local_size_x = 8) in;

layout(binding = 0) readonly buffer Input0 {
    float data[];
} weights;

layout(binding = 1) readonly buffer Input1 {
    float data[];
} previous_layer_out;

layout(binding = 2) writeonly buffer Output {
    float data[];
} output;

void main() {
    const int id = get_global_id(0);

    float t = 0;
    for (uint i = 0; i < 512; i++) {
        t += weights.data[i] * previous_layer_out.data[i];
    }

    t += 0.1;

    output.data[id] = sigmoid(t);
}