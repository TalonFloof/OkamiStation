#include <glad.c>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "karasuFB.h"

const char* vertexShader =
"#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 texCoord;\n"
"out vec2 outTexCoord;\n"
"void main() {\n"
"gl_Position = vec4(position,1.0);\n"
"outTexCoord = texCoord;\n"
"}"
;

const char* fragmentShader =
"#version 330\n"
"in vec2 outTexCoord;\n"
"out vec4 fragColor;\n"
"uniform sampler2D textureSampler;\n"
"uniform float TIME;"
"/* This shader is a modified version of henriquelalves's CRT Shader */\n"
"/* The shader can be found here: https://github.com/henriquelalves/SimpleGodotCRTShader/blob/master/addons/crt_shader/CRTShader.shader */\n"
"vec2 distort(vec2 p) {\n"
"float angle = p.y / p.x;\n"
"float theta = atan(p.y,p.x);\n"
"float radius = pow(length(p), 1.1 /*BarrelPower*/);\n"
"p.x = radius * cos(theta);\n"
"p.y = radius * sin(theta);\n"
"return 0.5 * (p + vec2(1.0,1.0));\n"
"}\n"
"void get_color_bleeding(inout vec4 current_color,inout vec4 color_left) {\n"
"current_color = current_color*vec4(1.2,0.5,1.0-1.2,1);\n"
"color_left = color_left*vec4(1.0-1.2,0.5,1.2,1);\n"
"}\n"
"void get_color_scanline(vec2 uv,inout vec4 c,float time) { /* The 768 is the screen height */ \n"
"float line_row = floor((uv.y * 768.0/2.0) + mod(time*30.0, 4.0));\n"
"float n = 1.0 - ceil((mod(line_row,4.0)/4.0));\n"
"c = c - n*c*(1.0 - 0.9);\n"
"c.a = 1.0;\n"
"}\n"
"\n"
"void main() {\n"
"vec2 xy = outTexCoord * 2.0;\n"
"xy.x -= 1.0;\n"
"xy.y -= 1.0;\n"
"float d = length(xy);\n"
"if(d < 1.5) {\n"
"xy = distort(xy);\n"
"} else {\n"
"xy = outTexCoord.xy;\n"
"}\n"
"float pixel_size_x = 1.0/1024*1;\n"
"float pixel_size_y = 1.0/768*1;\n"
"vec4 color_left = texture(textureSampler,vec2(xy.x,1.0-xy.y) - vec2(pixel_size_x, pixel_size_y));\n"
"vec4 current_color = texture(textureSampler,vec2(xy.x,1.0-xy.y));\n"
"get_color_bleeding(current_color,color_left);\n"
"vec4 c = current_color+color_left;\n"
"get_color_scanline(xy,c,TIME);\n"
"if(TIME < 5) {\n"
"fragColor = vec4(0.0,0.0,0.0,1.0);\n"
"} else {\n"
"fragColor = vec4(c.rgb * min(1.0,(TIME-5.0)/10.0),1.0);\n"
"}\n"
"}"
;

int main() {
    struct timeval timeVal;
    gettimeofday(&timeVal, 0);
    srandom(timeVal.tv_sec);
    KarasuInit();

    glfwInit();
    GLFWwindow* w = glfwCreateWindow(1024, 768, "OkamiStation", NULL, NULL);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwMakeContextCurrent(w);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize OpenGL context\n");
        return -1;
    }
    float t[] =
	{
		-1.f,  1.f, 0.0f,
		-1.f, -1.f, 0.0f,
		 1.f,  1.f, 0.0f,
         1.f,  1.f, 0.0f,
        -1.f, -1.f, 0.0f,
         1.f, -1.f, 0.0f,
	};
    float tex[] =
	{
		0.f, 1.f,
		0.f, 0.f,
		1.f, 1.f,
        1.f, 1.f,
        0.f, 0.f,
        1.f, 0.f,
	};
    uint32_t VAO;
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
    uint32_t PosVBO;
    uint32_t TexVBO;
    glGenBuffers(1,&PosVBO);
    glBindBuffer(GL_ARRAY_BUFFER, PosVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(t), (void*)&t, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glGenBuffers(1,&TexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, TexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tex), (void*)&tex, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    uint32_t texture;
    glGenTextures(1, &texture);

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertexShader, NULL);
    glCompileShader(vertShader);
    GLint status;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = calloc(length, sizeof(GLchar));
        glGetShaderInfoLog(vertShader, length, NULL, info);
        fprintf(stderr, "Vertex Shader failed:\n%s\n", info);
        free(info);
        glfwTerminate();
        return -1;
    }
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragmentShader, NULL);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = calloc(length, sizeof(GLchar));
        glGetShaderInfoLog(fragShader, length, NULL, info);
        fprintf(stderr, "Fragment Shader failed:\n%s\n", info);
        free(info);
        glfwTerminate();
        return -1;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = calloc(length, sizeof(GLchar));
        glGetProgramInfoLog(program, length, NULL, info);
        fprintf(stderr, "Program Linking failed: %s\n", info);
        free(info);
        glfwTerminate();
        return -1;
    }
    glDetachShader(program, vertShader);
    glDetachShader(program, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glUseProgram(program);
	glViewport(0, 0, 1024, 768);
    double beginTime = ((double)timeVal.tv_sec) + (((double)timeVal.tv_usec)/1000000.0);
    GLint timeUniform = glGetUniformLocation(program, "TIME");
    while(!glfwWindowShouldClose(w))
	{
        gettimeofday(&timeVal, 0);
        double elapsedTime = (((double)timeVal.tv_sec) + (((double)timeVal.tv_usec)/1000000.0)) - beginTime;
        glUniform1f(timeUniform, elapsedTime);

		glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 768, 0, GL_RGB, GL_UNSIGNED_BYTE, (unsigned char*)&GIMP_IMAGE_pixel_data);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindTexture(GL_TEXTURE_2D, 0);
        glfwSwapBuffers(w);
		glfwPollEvents();
	}
	glfwTerminate();
    return 0;
}