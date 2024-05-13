#include "FindMetalShaderPage.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach_debug/vm_info.h>

#include <sys/mman.h>

int proc_regionfilename(int pid, uint64_t address, void *buffer, uint32_t buffersize);

static void format_display_size(char buf[5], uint64_t size) {
    const char scale[] = { 'B', 'K', 'M', 'G', 'T', 'P', 'E' };
    double display_size = size;
    unsigned scale_index = 0;
    while (display_size >= 999.5) {
        display_size /= 1024;
        scale_index++;
    }
    int precision = 0;
    if (display_size < 9.95 && display_size - (float)((int)display_size) > 0) {
        precision = 1;
    }
    int len = snprintf(buf, 5, "%.*f%c", precision, display_size, scale[scale_index]);
}

static void format_memory_protection(char buf[4], int prot) {
    int len = snprintf(buf, 4, "%c%c%c",
            (prot & VM_PROT_READ    ? 'r' : '-'),
            (prot & VM_PROT_WRITE   ? 'w' : '-'),
            (prot & VM_PROT_EXECUTE ? 'x' : '-'));
}

static const char *share_mode_name(unsigned char share_mode) {
    switch (share_mode) {
        case SM_COW:                    return "COW";
        case SM_PRIVATE:                return "PRV";
        case SM_EMPTY:                  return "NUL";
        case SM_SHARED:                 return "ALI";
        case SM_TRUESHARED:             return "SHR";
        case SM_PRIVATE_ALIASED:        return "P/A";
        case SM_SHARED_ALIASED:         return "S/A";
        case SM_LARGE_PAGE:             return "LPG";
        default:                        return "???";
    }
}

void hex_dump(void* data, size_t size) {
    // Silently ignore silly per-line values.
    
    int perLine = 16;

    if (perLine < 4 || perLine > 64) perLine = 16;

    int i;
    unsigned char buff[perLine+1];
    const unsigned char * pc = (const unsigned char *)data;

    // Length checks.

    if (size == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (size < 0) {
        printf("  NEGATIVE LENGTH: %d\n", size);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < size; i++) {
        // Multiple of perLine means new or first line (with line offset).
        if ((i % perLine) == 0) {
            // Only print previous-line ASCII buffer for lines beyond first.
            if (i != 0) printf ("  %s\n", buff);
            
            // Output the offset of current line.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And buffer a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % perLine] = '.';
        else
            buff[i % perLine] = pc[i];
        buff[(i % perLine) + 1] = '\0';
    }

    // Pad out last line if not exactly perLine characters.
    while ((i % perLine) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII buffer.
    printf ("  %s\n", buff);
}

uint8_t* shader_patch = NULL;
size_t shader_patch_size = 0;

void read_shader_patch(char* path) {
    printf("Opening shader patch file: %s\n", path);
    
    FILE *f = fopen(path, "r");
    
    if (f == NULL) {
        printf("Could not open shader patch file\n");
    }
    
    fseek(f, 0L, SEEK_END);
    shader_patch_size = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    shader_patch = (uint8_t*)malloc(shader_patch_size);
    
    fread(&shader_patch[0], shader_patch_size, 1, f);
    fclose(f);
    
    hex_dump(&shader_patch[0], shader_patch_size);
}

void* find_agx_instruction_page(void) {
    void* agx_instruction_page = NULL;
    
    pid_t pid = getpid();
    
    uintptr_t start = 0;
    uintptr_t end = 0xFFFFFFFFFFFFFFFF;
    uint32_t depth = 2048;
    
    unsigned int found = 0;
    
    for (int first = 1;; first = 0) {
        mach_vm_address_t address = start;
        mach_vm_size_t size = 0;
        
        uint32_t depth0 = depth;
        vm_region_submap_info_data_64_t info;
        
        mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
        kern_return_t kr = mach_vm_region_recurse(mach_task_self(), &address, &size, &depth0, (vm_region_recurse_info_t)&info, &count);
        if (kr != KERN_SUCCESS || address > end) {
            if (first) {
                if (start == end) {
                    printf("no virtual memory region contains address %p\n",
                            (void *)start);
                } else {
                    printf("no virtual memory region intersects %p-%p\n",
                            (void *)start, (void *)end);
                }
            }
            break;
        }
        char vsize[5];
        format_display_size(vsize, size);
        char cur_prot[4];
        format_memory_protection(cur_prot, info.protection);
        char max_prot[4];
        format_memory_protection(max_prot, info.max_protection);
        
        // Get the file name for this region.
        char filename[4 + 4096] = {};
        memset(filename, ' ', 4);
        errno = 0;
        int ret = proc_regionfilename(pid, address, filename + 4, sizeof(filename) - 4);
        if (ret <= 0 || errno != 0) {
            filename[0] = 0;
        }

        printf("%016llx-%016llx [ %5s ] %s/%s %6s %5u %8u %6u %3u%s\n",
                address, address + size,
                vsize,
                cur_prot, max_prot,
                share_mode_name(info.share_mode),
                depth0,
                info.pages_resident,
                info.ref_count,
                info.user_tag,
                filename);
        
        if ((info.protection & VM_PROT_READ) != 0) {
            
            // Search for Metal shader in memory. We know that we hardcoded the value 0xab12cd34
            // in the shader, so we simply search every map for this value.
            for (uint8_t *p = (uint8_t*)address; p < (uint8_t*)(address + size - 4); p++) {
                if (*(uint32_t*)p == 0xab12cd34) {
                    printf("Found at: 0x%lx\n", (unsigned long)p);
                    hex_dump(p - 0x6, 32);
                    if (found != 0) {
                        *(uint32_t*)p = 0xab12cd36;
                    }
                    found++;
                    
                    /*
                    // Dump the currently used shader
                    FILE *f = fopen("out.bin", "w");
                    fwrite(p - 0x6, 0x14, 1, f);
                    fclose(f);*/
                    
                    if (shader_patch == NULL) {
                        *(uint32_t*)(p-2) = 0xcd3655e2;
                    }
                    else {
                        memcpy(p - 0x6, &shader_patch[0], shader_patch_size);
                    }
                    
                    hex_dump(p - 0x6, 32);
                    
                    if (found >= 6) {
                        return agx_instruction_page;
                    }
                }
            }
        }
        
        start = address + size;
    }
    
    return agx_instruction_page;
}
