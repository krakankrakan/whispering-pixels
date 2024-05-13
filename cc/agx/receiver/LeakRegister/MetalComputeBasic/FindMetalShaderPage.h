#ifndef FindMetalShaderPage_h
#define FindMetalShaderPage_h

#include <stdio.h>

void* find_agx_instruction_page(void);
void read_shader_patch(char* path);


void setStartTime();
void setEndTime();
unsigned long getExecutionTimeInNS();
double calculateGBperS(unsigned int* ptr);

#endif /* FindMetalShaderPage_h */
