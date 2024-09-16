import numpy as np
import struct

def export_float_array_to_binary(float_array, filename):
    with open(filename, "ab+") as file:
        for value in float_array:
            binary_data = struct.pack("f", value)
            file.write(binary_data)

wpe = np.load("wpe.npy")
wte = np.load("wte.npy")

print(wpe.shape)
print(wte.shape)

for i in range(1024):
    export_float_array_to_binary(wpe[i], "wpe.bin")

for i in range(50257):
    export_float_array_to_binary(wte[i], "wte.bin")