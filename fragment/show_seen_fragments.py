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
            float_value = struct.unpack('f', float_bytes)[0]
            float_array.append(float_value)
    
    return float_array

def find_correlating_fragments(fragments):
    for fragment in fragments:
        for other_fragment in fragments:
            if np.any(fragment != other_fragment):
                pass

if len(sys.argv) != 3:
    print("Arguments wrong")
    exit(-1)

orig_img_path = sys.argv[1]
leaked_fragments_path = sys.argv[2]

img = Image.open(orig_img_path).convert('RGB')

r, g, b = img.split()
r = np.asarray(r) / 255.0
g = np.asarray(g) / 255.0
b = np.asarray(b) / 255.0

image = r

print(r.shape)

#fig = plt.figure(figsize=(3, 1))
#plt.imshow(r)
#plt.title("r")
#plt.imshow(g)
#plt.title("g")
#plt.imshow(b)
#plt.title("b")
#plt.show()

leaked_floats = load_float_array_from_file(leaked_fragments_path)

# Iterate over leaked fragments & try to correlate the leaked fragment tiles to the original image
# ...
fragments, _, _, occurences = frag.convert_float_array_to_fragments(leaked_floats, num_groups=64)



window_sz_x = 8
window_sz_y = 4

# Find fragment shader in images

#rows = image.shape[0]
#cols = image.shape[1]

rows = int(image.shape[0] / window_sz_y)
cols = int(image.shape[1] / window_sz_x)

img_frag_corr_lists = [[[] for _ in range(cols)] for _ in range(rows)]

print("Creating fragment correlation lists")
for y in range(rows):
    for x in range(cols):

        image_x = x * window_sz_x
        image_y = y * window_sz_y

        print("{}, {}".format(x,y))

        window = image[image_y:image_y+window_sz_y, image_x:image_x+window_sz_x]

        corr_list = []

        for idx, fragment in enumerate(fragments):
            assert(window.shape == fragment.shape)

            #corr = np.corrcoef(fragment.flatten(), window.flatten())[0][1]
            corr = np.sum(np.abs(fragment.flatten() - window.flatten()))

            #print("({}, {}): corr: {}, idx: {}".format(x, y, max_corr, idx))
            #print(fragments[idx])

            corr_list.append(corr)
        
        img_frag_corr_lists[y][x] = corr_list

#img_frag_corr_array = np.array(img_frag_corr_lists)


# Create tuples of: (corr, fragment_idx, x, y)
print("Creating tuples")

corr_tupes = []
for y in range(rows):
    for x in range(cols):
        # Only get the fragment with the highest correlation
        #highest_corr = 0
        #highest_idx = None

        #for idx, corr in enumerate(img_frag_corr_lists[y][x]):
        #    if corr >= highest_corr:
        #        highest_corr = corr
        #        highest_idx = idx

        lowest_corr = 1000
        lowest_idx = None

        for idx, corr in enumerate(img_frag_corr_lists[y][x]):
            if corr <= lowest_corr:
                lowest_corr = corr
                lowest_idx = idx

        #for idx, corr in enumerate(img_frag_corr_lists[y][x]):
        corr_tupes.append(
            (
                lowest_corr,  #highest_corr,
                lowest_idx,   #highest_idx,
                x*window_sz_x,
                y*window_sz_y
            )
        )

# Print the 10 highest correlating fragments at their positions
print("Reconstructing image from tuples")

reconstructed_image = np.zeros(image.shape, dtype=np.float32)

corr_tupes.sort(key=lambda x: x[0])

seen_fragments_ctr = 0
for t in corr_tupes:
    corr = t[0]
    y = t[3]
    x = t[2]

    if corr > 0.01:
        continue

    print(t)


#    in_runs = []
#    for run_idx, run_fragment_entry in enumerate(run_fragment_idx_list):
#        for idx in run_fragment_entry:
#            if t[1] == idx:
#                in_runs.append(run_idx)
    #print(in_runs)

    #print(len(occurences))
    print(t[1])

    # Show number of occurences of single fragments
    #plt.text(x, y, str(occurences[t[1]]), color="red", fontsize=8, ha="center", va="center")

    if np.all(reconstructed_image[y:y+window_sz_y, x:x+window_sz_x] == 0):
        reconstructed_image[y:y+window_sz_y, x:x+window_sz_x] = fragments[t[1]]
        seen_fragments_ctr += 1

    # REMOVEME
    # For better reconsturction, we copy the last line of the fragment to the lower 8x4 pixels
    # in order to get "complete" fragments
    #if (y+window_sz_y+4 <= image.shape[0]):
    #    reconstructed_image[y+window_sz_y+0:y+window_sz_y+1, x:x+window_sz_x] = fragments[t[1]][3,:]
    #    reconstructed_image[y+window_sz_y+1:y+window_sz_y+2, x:x+window_sz_x] = fragments[t[1]][3,:]
    #    reconstructed_image[y+window_sz_y+2:y+window_sz_y+3, x:x+window_sz_x] = fragments[t[1]][3,:]
    #    reconstructed_image[y+window_sz_y+3:y+window_sz_y+4, x:x+window_sz_x] = fragments[t[1]][3,:]

    #plt.text()

print("seen fragments: " + str(seen_fragments_ctr))
print("total number of fragments: " + str(len(fragments)))

# Save image
saved_img = Image.fromarray(np.uint8(reconstructed_image * 255)).convert('RGB')
saved_img.resize((512,512), resample=Image.BOX)
saved_img.save("reconstructed_image.png")

#overlay_img = image * 0.3
overlay_img = reconstructed_image
plt.imshow(overlay_img, vmin=0.0, vmax=1.0, cmap="gray")
plt.show()
exit()


plt.subplot(1, 2, 1)
plt.imshow(reconstructed_image, vmin=0.0, vmax=1.0, cmap="gray")
plt.axis("off")
plt.subplot(1, 2, 2)
plt.imshow(image, vmin=0.0, vmax=1.0, cmap="gray")
plt.axis("off")
plt.tight_layout()
plt.show()

exit()

# Look up which fragment shaders are very similar
# by correlation
fragment_corr_list = []
for bf_idx, base_fragment in enumerate(fragments):
    corr_list = []

    for cf_idx, current_fragment in enumerate(fragments):

        corr = np.corrcoef(fragment.flatten(), window.flatten())

        corr_list.append(corr)
    
    fragment_corr_list.append(corr_list)

# Similarity by pixel variance
fragment_var_list = []
for fragment in fragments:
    fragment_var_list.append(np.var(fragment))

similarity_var_list = []
for frag_idx, fragment_var in enumerate(fragment_var_list):
    var_list = []

    for other_frag_idx, other_fragment_var in enumerate(fragment_var_list):

        if abs(fragment_var - other_fragment_var) < 0.0001:
            var_list.append(other_frag_idx)
    
    similarity_var_list.append(var_list)

# Get the amout of information contained in each fragment
information_list = []
for fragment in fragments:
    information = 0

    information_list.append(information)