#include <android/log.h>
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <string>

#define CONV_WEIGHTS_SIZE   5 * 5 * 8
#define FC_WEIGHTS_SIZE     8 * 12 * 12 * 10

#define INPUT_SIZE          28 * 28
#define CONV_OUTPUT_SIZE    24 * 24 * 8
#define RELU_OUTPUT_SIZE    24 * 24 * 8
#define POOL_OUTPUT_SIZE    12 * 12 * 8
#define OUTPUT_SIZE         10

float* conv_weights = nullptr;
float* fc_weights = nullptr;

float* input = nullptr;

static char* gConvComputeShader = NULL;
static char* gReluComputeShader = NULL;
static char* gPoolComputeShader = NULL;
static char* gFcComputeShader = NULL;

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

void setupSSBufferObject(GLuint& ssbo, GLuint index, void* pIn, GLuint count)
{
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(float), pIn, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, ssbo);
}

GLuint convComputeProgram;
GLuint reluComputeProgram;
GLuint poolComputeProgram;
GLuint fcComputeProgram;

GLuint inSSbo;
GLuint conv_weightsSSbo;
GLuint fc_weightsSSbo;

GLuint conv_outputSSbo;
GLuint relu_outputSSbo;
GLuint pool_outputSSbo;
GLuint fc_outputSSbo;

void conv_layer() {
    glUseProgram(convComputeProgram);
    CHECK();
    glDispatchCompute(8, 1, 1);
    CHECK();
    //glDeleteProgram(convComputeProgram);
    CHECK();
}

void relu_layer() {
    glUseProgram(reluComputeProgram);
    CHECK();
    glDispatchCompute(24, 1, 1);
    CHECK();
    //glDeleteProgram(reluComputeProgram);
    CHECK();
}

void pooling_layer() {
    glUseProgram(poolComputeProgram);
    CHECK();
    glDispatchCompute(8, 1, 1);
    CHECK();
    //glDeleteProgram(poolComputeProgram);
    CHECK();
}

void fc_layer() {
    glUseProgram(fcComputeProgram);
    CHECK();
    glDispatchCompute(1, 1, 1);
    CHECK();
    //glDeleteProgram(fcComputeProgram);
    CHECK();
}

void computeShader() {
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Creating conv program");
    convComputeProgram = createComputeProgram(gConvComputeShader);
    CHECK();
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Creating relu program");
    reluComputeProgram = createComputeProgram(gReluComputeShader);
    CHECK();
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Creating pool program");
    poolComputeProgram = createComputeProgram(gPoolComputeShader);
    CHECK();
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Creating fc program");
    fcComputeProgram = createComputeProgram(gFcComputeShader);
    CHECK();

    setupSSBufferObject(inSSbo, 0, input, INPUT_SIZE);
    CHECK();
    setupSSBufferObject(conv_weightsSSbo, 1, conv_weights, CONV_WEIGHTS_SIZE);
    CHECK();
    setupSSBufferObject(fc_weightsSSbo, 2, fc_weights, FC_WEIGHTS_SIZE);
    CHECK();
    setupSSBufferObject(conv_outputSSbo, 3, NULL, CONV_OUTPUT_SIZE);
    CHECK();
    setupSSBufferObject(relu_outputSSbo, 4, NULL, RELU_OUTPUT_SIZE);
    CHECK();
    setupSSBufferObject(pool_outputSSbo, 5, NULL, POOL_OUTPUT_SIZE);
    CHECK();
    setupSSBufferObject(fc_outputSSbo, 6, NULL, OUTPUT_SIZE);
    CHECK();

    //patch();

    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Running CNN");
    for (unsigned int j = 0; j < 10000; j++) {
        //__android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "conv");
        conv_layer();
        //__android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "relu");
        relu_layer();
        //__android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "pool");
        pooling_layer();
        //__android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "fc");
        fc_layer();

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        CHECK();

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, fc_outputSSbo);
        float *pOut = (float *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                 10 * sizeof(float), GL_MAP_READ_BIT);

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "verification PASSED\n");
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

extern "C" JNIEXPORT void JNICALL Java_com_example_neuralnetleakvictim_MainActivity_runComputeShader(JNIEnv *env, jobject thiz) {
    setupComputeShader();
}

float* loadFloatsFromFile(char* assetPath, AAssetManager* mgr) {
    float* assetData = nullptr;

    AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_STREAMING);

    if (asset) {
        off_t fileSize = AAsset_getLength(asset);
        assetData = new float[fileSize];

        int bytesRead = AAsset_read(asset, assetData, fileSize);
        AAsset_close(asset);

        if (bytesRead > 0) {
            __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"Done reading file");
        } else {
            __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Could not read shader file\n");
        }
    } else {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Could not open asset.\n");
    }

    return assetData;
}

char* loadShaderFromFile(char* assetPath, AAssetManager* mgr) {
    char* shaderSource = nullptr;

    AAsset* asset = AAssetManager_open(mgr, assetPath, AASSET_MODE_STREAMING);

    if (asset) {
        off_t fileSize = AAsset_getLength(asset);
        shaderSource = new char[fileSize + 1];
        shaderSource[fileSize] = '\0';

        int bytesRead = AAsset_read(asset, shaderSource, fileSize);
        AAsset_close(asset);

        if (bytesRead > 0) {
            __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"glsl: %s\n", shaderSource);
        } else {
            __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Could not read shader file\n");
        }
    } else {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Could not open asset.\n");
    }

    return shaderSource;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_neuralnetleakvictim_MainActivity_loadShaders(JNIEnv* env, jobject /* this */, jobject assetManager) {
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"Loading shaders..\n");

    /*
     * Load assets from filesystem
     */
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    if (mgr == NULL) {
        __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"mgr is NULL\n");
    }

    gConvComputeShader = loadShaderFromFile("conv.glsl", mgr);
    gReluComputeShader = loadShaderFromFile("relu.glsl", mgr);
    gPoolComputeShader = loadShaderFromFile("pool.glsl", mgr);
    gFcComputeShader = loadShaderFromFile("fc.glsl", mgr);

    /*
     * Initialize weights & input
     */
    conv_weights = loadFloatsFromFile("conv_weights.bin", mgr);
    fc_weights = loadFloatsFromFile("fc_weights.bin", mgr);
    input = loadFloatsFromFile("conv_input.bin", mgr);

    // Add some noise to the input
    for (unsigned int i = 0; i < INPUT_SIZE; i++) {
        input[i] += ((float)rand()/(float)(RAND_MAX)) * 0.0001;
    }

    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__, "Loaded weights and input.\n");
}
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(ANDROID_LOG_INFO,  __FUNCTION__,"Hello from shared library!\n");

    return JNI_VERSION_1_6;
}