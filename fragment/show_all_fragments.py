from PIL import Image
import sys
import struct
import numpy as np
from matplotlib import pyplot as plt
import frag2 as frag
import math

def load_float_array_from_file(filename):
    float_array = []
    with open(filename, 'rb') as file:
        while True:
            float_bytes = file.read(4)
            if not float_bytes:
                break
            float_value = struct.unpack('f', float_bytes)[0]
            float_array.append(float_value)
    
    return float_array

leaked_fragments_path = sys.argv[1]
leaked_floats = load_float_array_from_file(leaked_fragments_path)
fragments, _, _, occurences = frag.convert_float_array_to_fragments(leaked_floats, num_groups=64)

# Store all fragments in one big image
size = math.ceil(math.sqrt(len(fragments)))
print("image size: " + str(size))
all_fragments_img = np.zeros((size*4, size*8), dtype=np.float32)

for x in range(0, size):
    for y in range(0, size):
        current_fragment_idx = x + y*size
        #print(len(fragments))
        #print(current_fragment_idx)
        if current_fragment_idx >= len(fragments):
            continue
        frag = fragments[current_fragment_idx]
        all_fragments_img[4*y : 4*(y+1), 8*x : 8*(x+1)] = frag

all_fragments_img_pil = Image.fromarray(np.uint8(all_fragments_img * 255)).convert('RGB')
all_fragments_img_pil = all_fragments_img_pil.resize((all_fragments_img.shape[1] * 8, all_fragments_img.shape[0] * 8), resample=Image.BOX)
print(all_fragments_img_pil.size)
all_fragments_img_pil.save("show_all.png")

plt.imshow(all_fragments_img, vmin=0.0, vmax=1.0, cmap="gray")
plt.show()