#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import "MetalAdder.h"
#import <stdlib.h>

extern unsigned int arrayLength;
extern unsigned int bufferSize;

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();

        // Create the custom object used to encapsulate the Metal code.
        // Initializes objects to communicate with the GPU.
        MetalAdder* adder = [[MetalAdder alloc] initWithDevice:device];
        
        // Create buffers to hold data
        [adder prepareData];
        
        // Send a command to the GPU to perform the calculation.
        while(1) {
            printf("Sending via covert channel\n");
            [adder sendComputeCommand];
        }
    }
    return 0;
}
