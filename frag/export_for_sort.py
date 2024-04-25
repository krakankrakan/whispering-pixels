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

leaked_floats = load_float_array_from_file(sys.argv[1])
fragments_1, _, _, _ = frag.convert_float_array_to_fragments(leaked_floats, num_groups=64)

print(len(fragments_1))

#import math
# Store all not seen fragments in one big image
#size = 24 #math.ceil(math.sqrt(len(fragments_1)))
#print("image size: " + str(size))

all_fragments_img = np.zeros((32, 32), dtype=np.float32)

current_fragment_idx = 0
for x in range(0, 4):
    for y in range(0, 8):
        frag = fragments_1[current_fragment_idx]
        all_fragments_img[4*y : 4*(y+1), 8*x : 8*(x+1)] = frag

        current_fragment_idx += 1

all_fragments_img_pil = Image.fromarray(np.uint8(all_fragments_img * 255)).convert('RGB')
#min_val = min(all_fragments_img_pil.getdata())
#max_val = max(all_fragments_img_pil.getdata())
#scaling_factor = 255.0 / (max_val[0] - min_val[0])
#all_fragments_img_pil = all_fragments_img_pil.point(lambda x: (x - min_val[0]) * scaling_factor)

#all_fragments_img_pil = all_fragments_img_pil.resize((1024, 1024), resample=Image.NONE)
print(all_fragments_img_pil.size)
#all_fragments_img_pil = all_fragments_img_pil.resize((2048, 2048), Image.NEAREST)
all_fragments_img_pil.save("exported.png")

plt.imshow(all_fragments_img, vmin=0.0, vmax=1.0, cmap="gray")
plt.show()