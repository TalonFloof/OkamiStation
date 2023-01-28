#include <glad.c>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>

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
"/* This shader is a modified version of Hazukiaoi's CRT Shader */\n"
"/* The shader can be found here: https://github.com/Hazukiaoi/CRTMonitorShader/blob/master/Shader/CRT.shader */\n"
"float sdLine(in vec2 p, in vec2 a, in vec2 b) {\n"
"vec2 pa = p - a, ba = b - a;\n"
"float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);\n"
"return length(pa - ba * h);\n"
"}\n"
"\n"
"void main() {\n"
"vec2 scaleDir = normalize(outTexCoord - vec2(0.5, 0.5));\n"
"float _scale = distance(outTexCoord, vec2(0.5, 0.5));\n"
"vec2 scaledUV = outTexCoord + scaleDir * pow(max(0, _scale),3) * 0.5 /* Round Scale */;\n"
"vec2 uv = scaledUV * 50.0 /* Count */;\n"
"\n"
"vec4 _mainTex = texture(textureSampler, round( scaledUV * 50.0 /* Count */) / 50.0 /* Count */);\n"
"vec3 lineSize = clamp(_mainTex.rgb,0.0,1.0) * 0.35;\n"
"vec3 smoothSizeOffset = (1 - clamp(_mainTex.rgb,0.0,1.0)) * 0.1;\n"
"vec2 smoothSize = vec2(0.1, 0.2);\n"
"float dr = 1 - smoothstep(smoothSize.x - smoothSizeOffset.r, smoothSize.y + smoothSizeOffset.r, sdLine(uv, vec2(0.2, 0.5 - lineSize.r), vec2(0.2, 0.5 + lineSize.r)));\n"
"float dg = 1 - smoothstep(smoothSize.x - smoothSizeOffset.g, smoothSize.y + smoothSizeOffset.g, sdLine(uv, vec2(0.5, 0.5 - lineSize.g), vec2(0.5, 0.5 + lineSize.g)));\n"
"float db = 1 - smoothstep(smoothSize.x - smoothSizeOffset.b, smoothSize.y + smoothSizeOffset.b, sdLine(uv, vec2(0.8, 0.5 - lineSize.b), vec2(0.8, 0.5 + lineSize.b)));\n"
"float mask = (smoothstep(0.0, 0.01, scaledUV.x)) * (1 - smoothstep(0.99, 1.0, scaledUV.x)) * (smoothstep(0.0, 0.01, scaledUV.y)) * (1 - smoothstep(0.99, 1.0, scaledUV.y));\n"
"vec4 findColor = vec4(dr, dg, db, 1) * _mainTex;\n"
"findColor.a = mask;\n"
"fragColor = findColor;\n"
"}"
;

int main() {
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
    uint32_t v;
    glGenBuffers(1, &v);
    glBindBuffer(GL_ARRAY_BUFFER, v);
    glBufferData(GL_ARRAY_BUFFER, sizeof(t)*sizeof(t)/sizeof(t[0]),
		&t[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3,
		 (void*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
        sizeof(float) * 10, (void*)(sizeof(float) * 6));
    glEnableVertexAttribArray(0);
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
    while(!glfwWindowShouldClose(w))
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	glfwTerminate();
    return 0;
}