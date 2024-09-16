import numpy as np
import math

def remove_list_with_with_same_element(s):
    filtered_set = set()
    
    for lst in s:
        unique_values = set(lst)
        if len(unique_values) > 1:
            filtered_set.add(lst)
    
    return filtered_set

def convert_frag_to_float_array(frag):
    unique_chunk = np.zeros((32,))

    unique_chunk[0] = frag[0][0]
    unique_chunk[1] = frag[0][1]
    unique_chunk[4] = frag[0][2]
    unique_chunk[5] = frag[0][3]

    unique_chunk[2] = frag[1][0]
    unique_chunk[3] = frag[1][1]
    unique_chunk[6] = frag[1][2]
    unique_chunk[7] = frag[1][3]

    unique_chunk[8] = frag[0][4 + 0]
    unique_chunk[9] = frag[0][4 + 1]
    unique_chunk[12] = frag[0][4 + 2]
    unique_chunk[13] = frag[0][4 + 3]

    unique_chunk[10] = frag[1][4 + 0]
    unique_chunk[11] = frag[1][4 + 1]
    unique_chunk[14] = frag[1][4 + 2]
    unique_chunk[15] = frag[1][4 + 3]

    unique_chunk[16 + 0] = frag[2][0]
    unique_chunk[16 + 1] = frag[2][1]
    unique_chunk[16 + 4] = frag[2][2]
    unique_chunk[16 + 5] = frag[2][3]

    unique_chunk[16 + 2] = frag[3][0]
    unique_chunk[16 + 3] = frag[3][1]
    unique_chunk[16 + 6] = frag[3][2]
    unique_chunk[16 + 7] = frag[3][3]

    unique_chunk[16 + 8] = frag[2][4 + 0]
    unique_chunk[16 + 9] = frag[2][4 + 1]
    unique_chunk[16 + 12] = frag[2][4 + 2]
    unique_chunk[16 + 13] = frag[2][4 + 3]

    unique_chunk[16 + 10] = frag[3][4 + 0]
    unique_chunk[16 + 11] = frag[3][4 + 1]
    unique_chunk[16 + 14] = frag[3][4 + 2]
    unique_chunk[16 + 15] = frag[3][4 + 3]

    return unique_chunk


def convert_float_array_to_fragments(float_array, num_groups=16, orig_img=None):
    chunks = []
    for i in range(0, int(len(float_array)), 32):
        current_chunk = float_array[i : i+32]

        #print(current_chunk)

        chunks.append(current_chunk)

    chunks_set = {tuple(row) for row in chunks}

    print("Number of chunks:")
    print(len(chunks_set))
    print("")

    chunks_set_seen_often = set()

    occurences = []
#    for unique_chunk in chunks_set:
#        occurence_ctr = 0
#
#        for c in chunks:
#            if unique_chunk == tuple(c):
#                occurence_ctr+=1
#        
#        print("#occurences: " + str(occurence_ctr))
#        print(unique_chunk)
#        print("")
#
#        if occurence_ctr > 30:
#            chunks_set_seen_often.add(unique_chunk)
#        
#        occurences.append(occurence_ctr)

    #chunks_set = chunks_set_seen_often

    # Remove all chunks with equal values
    chunks_set = remove_list_with_with_same_element(chunks_set)

    axis_sz = int(math.sqrt(len(chunks_set))) + 1




#    # Generate the indices list which maps single leak runs to fragment indices
#    # Maps fragments to runs
#    runs = []
#    for i in range(0, len(float_array), num_groups*256):
#        run_floats = float_array[i : i+num_groups*256]
#
#        run = []
#
#        for j in range(0, len(run_floats), 32):
#            current_chunk = run_floats[j : j+32]
#            run.append(current_chunk)
#        
#        runs.append(run)
#
#    #print(len(float_array))
#    #print(int(len(float_array) / (16*256)))
#    print(len(runs))
#    #print(runs)
#    #exit()
#
    run_fragment_idx_list = []
#    for run in runs:
#        idx_list = []
#
#        for fragment in run:
#            for idx, unique_chunk in enumerate(list(chunks_set)):
#                if tuple(fragment) == unique_chunk:
#                    idx_list.append(idx)
#        
#        run_fragment_idx_list.append(idx_list)
#
#    #run_fragment_idx_list = [idx_list[i:i + (16)] for i in range(0, len(idx_list), (16))]
#    #print(len(idx_list))
#    print(len(run_fragment_idx_list))
#
#    for run_fragment_idx_list_entry in run_fragment_idx_list:
#        #print(sorted(run_fragment_idx_list_entry))
#        print(run_fragment_idx_list_entry)
#
#    #print(run_fragment_idx_list[0])
#    #print(runs[0])
#    #exit()


    fragments = []
    for unique_chunk in chunks_set:
        frag = np.zeros((4, 8), dtype=np.float32)

        frag[0][0] = unique_chunk[0]
        frag[0][1] = unique_chunk[1]
        frag[0][2] = unique_chunk[4]
        frag[0][3] = unique_chunk[5]

        frag[1][0] = unique_chunk[2]
        frag[1][1] = unique_chunk[3]
        frag[1][2] = unique_chunk[6]
        frag[1][3] = unique_chunk[7]

        frag[0][4 + 0] = unique_chunk[8]
        frag[0][4 + 1] = unique_chunk[9]
        frag[0][4 + 2] = unique_chunk[12]
        frag[0][4 + 3] = unique_chunk[13]

        frag[1][4 + 0] = unique_chunk[10]
        frag[1][4 + 1] = unique_chunk[11]
        frag[1][4 + 2] = unique_chunk[14]
        frag[1][4 + 3] = unique_chunk[15]


        frag[2][0] = unique_chunk[16 + 0]
        frag[2][1] = unique_chunk[16 + 1]
        frag[2][2] = unique_chunk[16 + 4]
        frag[2][3] = unique_chunk[16 + 5]

        frag[3][0] = unique_chunk[16 + 2]
        frag[3][1] = unique_chunk[16 + 3]
        frag[3][2] = unique_chunk[16 + 6]
        frag[3][3] = unique_chunk[16 + 7]

        frag[2][4 + 0] = unique_chunk[16 + 8]
        frag[2][4 + 1] = unique_chunk[16 + 9]
        frag[2][4 + 2] = unique_chunk[16 + 12]
        frag[2][4 + 3] = unique_chunk[16 + 13]

        frag[3][4 + 0] = unique_chunk[16 + 10]
        frag[3][4 + 1] = unique_chunk[16 + 11]
        frag[3][4 + 2] = unique_chunk[16 + 14]
        frag[3][4 + 3] = unique_chunk[16 + 15]

        fragments.append(frag)
    
    return fragments, None, None, None
    
    # Debug stuff
    og_img_fragments = []
    for r in range(0, orig_img.shape[0], 4):
        for c in range(0, orig_img.shape[1], 8):
            subarray = orig_img[r:r + 4, c:c + 8]
            og_img_fragments.append(subarray)
    
    new_fragments = fragments

    print(len(fragments))
    print(orig_img.shape)
    print(len(og_img_fragments))
    print("1\n")

    for i, og_img_fragment in enumerate(og_img_fragments):
        og_img_fragment_np = np.array(og_img_fragment, dtype=np.float32)
        #print(og_img_fragment)

        found = False
        for frag in fragments:
            frag = np.array(frag, dtype=np.float32)
            #print(frag)
            #a = og_img_fragment_np - frag
            #print(a)
            
            if np.all(np.abs(og_img_fragment_np - frag) < 0.000001):
                # image fragment already in leaked fragment list
                print("found")
                print(og_img_fragment_np)
                print(frag)
                found = True
                break

        if not found:
            # Add image fragment to fragment list
            new_fragments.append(og_img_fragment_np)

    fragments = new_fragments

    print(len(fragments))
    print("Done 2")

    return fragments, axis_sz, run_fragment_idx_list, occurences