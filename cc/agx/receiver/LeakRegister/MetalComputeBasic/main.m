#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import "MetalAdder.h"
#import "FindMetalShaderPage.h"
#import <stdlib.h>

int main(int argc, const char * argv[]) {
    
    char *p = NULL;
    
    if (argc == 1) {
        p = "patch.bin";
    } else {
        p = argv[1];
    }

    @autoreleasepool {
        read_shader_patch(p);
        
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();

        // Create the custom object used to encapsulate the Metal code.
        // Initializes objects to communicate with the GPU.
        MetalAdder* adder = [[MetalAdder alloc] initWithDevice:device];
        
        printf("Looking for AGX instruction page\n");
        find_agx_instruction_page();
        printf("Done!\n");
        
        // Create buffers to hold data
        [adder prepareData];
        
        // Send a command to the GPU to perform the calculation.
        printf("Executing covert channel receiver shader...\n");
        while(1) {
            printf("Receiving via covert channel\n");
            
            [adder sendComputeCommand];
        }
    }
    return 0;
}
