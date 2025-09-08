#pragma once
#include <glad/glad.h>
#include <string>
#include <cstdio>

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

static inline void APIENTRY glDbg(GLenum source, GLenum type, GLuint id,
                                  GLenum severity, GLsizei length,
                                  const GLchar* msg, const void* user)
{
    (void)source; (void)type; (void)id; (void)severity; (void)length; (void)user;
    std::fprintf(stderr, "[GL] %s\n", msg);
}

GLuint compileShader(GLenum type, const char* src, const char* debugName=nullptr);
GLuint linkProgram(GLuint vs, GLuint fs);
GLuint createCompute(const std::string& path);
GLuint createProgramFromFiles(const std::string& vsPath,
                              const std::string& fsPath);
GLuint createTex2D_R32F(int w,int h);
GLuint createTex2D_RG32F(int w,int h);
GLuint createTex2D_R8UI(int w,int h);
GLuint createTex2DArray_R32F(int w,int h,int layers);
void   bindImage(GLuint tex, GLuint unit, GLenum access, GLenum format, int layer=-1);
void   dispatch2D(int w,int h,int lx=16,int ly=16);
void   checkGLErr(const char* where=nullptr);