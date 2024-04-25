from PIL import Image
import sys
import struct
import numpy as np
from matplotlib import pyplot as plt
import frag2 as frag

def load_float_array_from_file(filename):
    float_array = []
    with open(filename, 'rb') as file:
        while True:
            float_bytes = file.read(4)
            if not float_bytes:
                break
            float_value = struct.unpack("f", float_bytes)[0]
            float_array.append(float_value)
    return float_array

def write_float_array_to_file(filename, float_array):
    with open(filename, 'wb') as file:
        for f in float_array:
            file.write(struct.pack("f", f))

def convert_float_array_to_chunks(float_array):
    chunks = []

    for i in range(0, int(len(float_array)), 32):
        current_chunk = float_array[i : i+32]

        #print(current_chunk)

        chunks.append(current_chunk)

    chunks_set = {tuple(row) for row in chunks}

    return list(chunks_set)

if len(sys.argv) != 4:
    print("Arguments wrong")
    exit(-1)

merge_file_1 = sys.argv[1]
merge_file_2 = sys.argv[2]

out_file = sys.argv[3]

leaked_floats = load_float_array_from_file(merge_file_1)
fragments_1 = convert_float_array_to_chunks(leaked_floats)

leaked_floats = load_float_array_from_file(merge_file_2)
fragments_2 = convert_float_array_to_chunks(leaked_floats)

out_fragments = []

for fragment_1 in fragments_1:
    for fragment_2 in fragments_2:
        if np.all(fragment_1 == fragment_2):
            out_fragments.append(fragment_1)

print(len(out_fragments))

# Eliminate invalid fragments
valid_frags = []
for frag in out_fragments:
    valid = True

    if np.all(frag < tuple([0.000001] * 32)):
        valid = False
    
    if np.abs(np.sum(np.array(list(frag))) - frag[0]) < 0.001:
        valid = False

    if len(set(list(frag))) < 4:
        valid = False
    
    if valid:
        valid_frags.append(frag)

print(len(valid_frags))

write_float_array_to_file(out_file, np.array(valid_frags).flatten())