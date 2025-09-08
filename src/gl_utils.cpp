#include "gl_utils.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// ---------- local helpers ----------
static std::string readFileText(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "[io] cannot open %s\n", path);
        return {};
    }
    std::vector<char> buf((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
    return std::string(buf.begin(), buf.end());
}

static GLuint compileShader(GLenum type, const char* src, const char* dbgName) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetShaderInfoLog(sh, 4096, nullptr, log);
        std::fprintf(stderr, "[shader compile] %s\n%s\n", dbgName?dbgName:"", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint linkProgram(std::initializer_list<GLuint> shaders) {
    GLuint p = glCreateProgram();
    for (auto s : shaders) glAttachShader(p, s);
    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetProgramInfoLog(p, 4096, nullptr, log);
        std::fprintf(stderr, "[link] %s\n", log);
        glDeleteProgram(p);
        p = 0;
    }
    for (auto s : shaders) glDeleteShader(s);
    return p;
}

// ---------- public API ----------
GLuint createCompute(const std::string& path) {
    std::string src = readFileText(path.c_str());
    if (src.empty()) return 0;
    GLuint cs = compileShader(GL_COMPUTE_SHADER, src.c_str(), path.c_str());
    return cs ? linkProgram({ cs }) : 0;
}

GLuint createProgramFromFiles(const std::string& vsPath,
                              const std::string& fsPath) {
    std::string vsrc = readFileText(vsPath.c_str());
    std::string fsrc = readFileText(fsPath.c_str());
    if (vsrc.empty() || fsrc.empty()) return 0;

    GLuint vs = compileShader(GL_VERTEX_SHADER,   vsrc.c_str(), vsPath.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc.c_str(), fsPath.c_str());
    if (!vs || !fs) { if (vs) glDeleteShader(vs); if (fs) glDeleteShader(fs); return 0; }

    //attribute 0 ชื่อ aPos
    GLuint prog = linkProgram({ vs, fs });
    if (prog) glBindAttribLocation(prog, 0, "aPos"); // จะมีผลตอน link; ถ้าต้องการชัวร์ก็ bind ก่อน link
    return prog;
}
