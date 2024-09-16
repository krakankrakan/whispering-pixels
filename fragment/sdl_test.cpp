#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/glew.h>
#include <GL/glu.h>

#include <string.h>
#include <sys/mman.h>

static const char* vertexShaderCode = "           \n\
#version 120                                      \n\
                                                  \n\
attribute vec3 Position;                          \n\
attribute vec2 UV;                                \n\
varying vec2 outUV;                               \n\
                                                  \n\
void main() {                                     \n\
	vec2 pos = Position.xy - vec2(100, 100);  \n\
	pos /= vec2(100, -100);                   \n\
	gl_Position = vec4(pos, 0, 1);            \n\
                                                  \n\
	outUV = UV;                               \n\
}";

/*static const char* fragmentShaderCode = "         \n\
#version 120                                      \n\
                                                  \n\
varying vec2 outUV;                               \n\
uniform sampler2D sampler;                        \n\
                                                  \n\
void main() {                                     \n\
	vec4 ret = texture2D(sampler, outUV); \n\
	ret.g = 0.1237 + outUV.y * 0.001;\n\
	gl_FragColor = ret;\n\
}";*/

static const char* fragmentShaderCode = "         \n\
#version 120                                      \n\
                                                  \n\
varying vec2 outUV;                               \n\
uniform sampler2D sampler;                        \n\
                                                  \n\
void main() {                                     \n\
	vec4 ret = texture2D(sampler, outUV); \n\
	ret.r = 0.5555;\n\
	ret.g *= 1.1;\n\
	ret.b = 0.1237;\n\
	gl_FragColor = ret;\n\
}";

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

unsigned int ctr = 0;
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

            uint64_t* ptr = (uint64_t*)start;

            while(*ptr != 0x0) ptr++;

            shader_size = (unsigned long)ptr - (unsigned long)start;

			char shader_dump_path[64];
			sprintf(shader_dump_path, "./shader_%d.bin", ctr);
			printf("Dumping to file: %s\n", shader_dump_path);

            //hexDump((void *) start, shader_size, NULL);
			hexDump((void *) start, 0x100, NULL);

			FILE *f = fopen(shader_dump_path, "w+");
			fwrite((void *) start, shader_size, 1, f);
			fclose(f);

			ctr++;
        }
    }
}

SDL_Window * window;
SDL_GLContext glContext;

void init() {
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("Texture Example in OpenGL",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		200, 200,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	glContext = SDL_GL_CreateContext(window);

    int imgFlags = IMG_INIT_PNG;
	if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
        printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
    }

	glewInit();
}

GLuint createShader(const char * shaderCode, GLenum shaderType) {
	GLuint shader = glCreateShader(shaderType);

	const GLchar* strings[] = { shaderCode };	// shader code strings
	GLint lengths[] = { (GLint)strlen(shaderCode) };	// shader code string length

	glShaderSource(shader, 1, strings, lengths);
	glCompileShader(shader);

	return shader;
}

GLuint createProgram(const char * vertexShaderCode, const char * fragmentShaderCode) {
	GLuint program = glCreateProgram();

	glAttachShader(program, createShader(vertexShaderCode, GL_VERTEX_SHADER));
	glAttachShader(program, createShader(fragmentShaderCode, GL_FRAGMENT_SHADER));
	glLinkProgram(program);
	glValidateProgram(program);

	return program;
}

GLuint createGLTextureFromSurface(SDL_Surface * surface) {
	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

void drawGLTexture(GLuint textureID, float x, float y, int textureW, int textureH) {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	GLuint textureVBO;
	glGenBuffers(1, &textureVBO);
	glBindBuffer(GL_ARRAY_BUFFER, textureVBO);

	GLfloat textureVertexData[] = {
		x, y, 0, 0,
		x, y + textureH, 0, 1.0f,
		x + textureW, y, 1.0f, 0,

		x, y + textureH, 0, 1.0f,
		x + textureW, y + textureH, 1.0f, 1.0f,
		x + textureW, y, 1.0f, 0
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureVertexData), textureVertexData, GL_STATIC_DRAW);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);	// x,y repeat every 4 values

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 4 * sizeof(GLfloat), (const GLvoid*)(2 * sizeof(GLfloat)));	// u,v start at 2, repeat 4

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glDeleteBuffers(1, &textureVBO);
}

int main(int argc, char * args[]) {
	init();

	GLuint program = createProgram(vertexShaderCode, fragmentShaderCode);
	glUseProgram(program);

	SDL_Surface * texture = IMG_Load("parrot32.png");
    if( texture == NULL )
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", "font.jpg", IMG_GetError() );
    }
	GLuint textureID = createGLTextureFromSurface(texture);
	SDL_FreeSurface(texture);


	int printed = 0;
    while(1) {
		drawGLTexture(textureID, 0, 0, 32, 32);	
		if (printed == 0) {
			//patch();
			printed = 1;
		}
		SDL_GL_SwapWindow(window);
    }

	SDL_Delay(1000);

	glDeleteTextures(1, &textureID);

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}
