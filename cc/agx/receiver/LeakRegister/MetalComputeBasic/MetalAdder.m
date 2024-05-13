#import "MetalAdder.h"
#import "FindMetalShaderPage.h"
#import "FindMetalShaderPage.hpp"

#define THREADGROUPS    2
#define THREADGROUPSIZE 1024

// The number of floats in each array, and the size of the arrays in bytes.
const unsigned int arrayLength = THREADGROUPS * THREADGROUPSIZE;
const unsigned int bufferSize = arrayLength * sizeof(float);

@implementation MetalAdder
{
    id<MTLDevice> _mDevice;

    // The compute pipeline generated from the compute kernel in the .metal shader file.
    id<MTLComputePipelineState> _mAddFunctionPSO;

    // The command queue used to pass commands to the device.
    id<MTLCommandQueue> _mCommandQueue;

    // Buffers to hold data.
    id<MTLBuffer> _mBufferA;
    id<MTLBuffer> _mBufferB;
    id<MTLBuffer> _mBufferResult;

}

- (instancetype) initWithDevice: (id<MTLDevice>) device
{
    self = [super init];
    if (self)
    {
        _mDevice = device;

        NSError* error = nil;

        // Load the shader files with a .metal file extension in the project

        id<MTLLibrary> defaultLibrary = [_mDevice newDefaultLibrary];
        if (defaultLibrary == nil)
        {
            NSLog(@"Failed to find the default library.");
            return nil;
        }

        id<MTLFunction> addFunction = [defaultLibrary newFunctionWithName:@"test_shader"];
        if (addFunction == nil)
        {
            NSLog(@"Failed to find the adder function.");
            return nil;
        }

        // Create a compute pipeline state object.
        _mAddFunctionPSO = [_mDevice newComputePipelineStateWithFunction: addFunction error:&error];
        if (_mAddFunctionPSO == nil)
        {
            //  If the Metal API validation is enabled, you can find out more information about what
            //  went wrong.  (Metal API validation is enabled by default when a debug build is run
            //  from Xcode)
            NSLog(@"Failed to created pipeline state object, error %@.", error);
            return nil;
        }

        _mCommandQueue = [_mDevice newCommandQueue];
        if (_mCommandQueue == nil)
        {
            NSLog(@"Failed to find the command queue.");
            return nil;
        }
    }

    return self;
}

- (void) prepareData
{
    // Allocate three buffers to hold our initial data and the result.
    _mBufferA = [_mDevice newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
    _mBufferB = [_mDevice newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
    _mBufferResult = [_mDevice newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];

    [self generateDeterministicIntData:_mBufferA];
    [self generateDeterministicIntData:_mBufferB];
}

- (void) sendComputeCommand
{
    // Zero the result buffer
    int* resultDataPtr = _mBufferResult.contents;

    for (unsigned long index = 0; index < arrayLength; index++)
    {
        resultDataPtr[index] = 0;
    }
    
    // Create a command buffer to hold commands.
    id<MTLCommandBuffer> commandBuffer = [_mCommandQueue commandBuffer];
    assert(commandBuffer != nil);

    // Start a compute pass.
    id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];
    assert(computeEncoder != nil);

    [self encodeAddCommand:computeEncoder];

    // End the compute pass.
    [computeEncoder endEncoding];

    // Execute the command.
    setStartTime();
    [commandBuffer commit];
    setEndTime();
    
    calculateGBperS((unsigned int*)_mBufferResult.contents);

    // Normally, you want to do other work in your app while the GPU is running,
    // but in this example, the code simply blocks until the calculation is complete.
    //[commandBuffer waitUntilCompleted];

    unsigned int exec_time = getExecutionTimeInNS();

    printf("%ld ns\n", exec_time);

    //[self verifyResults];
}

- (void)encodeAddCommand:(id<MTLComputeCommandEncoder>)computeEncoder {

    // Encode the pipeline state object and its parameters.
    [computeEncoder setComputePipelineState:_mAddFunctionPSO];
    [computeEncoder setBuffer:_mBufferA offset:0 atIndex:0];
    [computeEncoder setBuffer:_mBufferB offset:0 atIndex:1];
    [computeEncoder setBuffer:_mBufferResult offset:0 atIndex:2];

    MTLSize gridSize = MTLSizeMake(arrayLength, 1, 1);

    // Calculate a threadgroup size.
    NSUInteger threadGroupSize = _mAddFunctionPSO.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > arrayLength)
    {
        threadGroupSize = arrayLength;
    }
    MTLSize threadgroupSize = MTLSizeMake(threadGroupSize, 1, 1);

    // Encode the compute command.
    [computeEncoder dispatchThreads:gridSize
              threadsPerThreadgroup:threadgroupSize];
}

- (void) generateDeterministicIntData: (id<MTLBuffer>) buffer
{
    int* dataPtr = buffer.contents;

    for (unsigned long index = 0; index < arrayLength; index++)
    {
        dataPtr[index] = 0;
    }
}

- (void) verifyResults
{
    int* result = _mBufferResult.contents;

    for (unsigned long index = 0; index < arrayLength; index++)
    {
        if (result[index] == 0xab12cd34)
        {
            printf("Successfully leaked data: index=%lu result=0x%x\n", index, result[index]);
        }
    }
}
@end
