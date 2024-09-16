import struct
import numpy as np
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import sys
import math

def load_array_from_file(filename):
    array = []
    with open(filename, 'rb') as file:
        while True:
            value_bytes = file.read(4)  # Read 4 bytes for a float value
            if not value_bytes:
                break  # Reached the end of the file
            value = struct.unpack('f', value_bytes)[0]
            array.append(value)
    return array

out  = load_array_from_file(sys.argv[1])

height = int(math.floor(len(out) / 24))
print(height)
out = out[:24*height]

# Method 1
pixels = np.array(out).reshape(height, 24)
#pixels = np.array(orig_out).reshape(24, 192)

plt.imshow(pixels, cmap='gray') 
plt.show()