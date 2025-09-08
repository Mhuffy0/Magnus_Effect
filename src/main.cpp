#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

// ================== small math ==================
struct Vec2 { float x,y; };
static inline Vec2 operator+(Vec2 a, Vec2 b){ return {a.x+b.x,a.y+b.y}; }
static inline Vec2 operator-(Vec2 a, Vec2 b){ return {a.x-b.x,a.y-b.y}; }
static inline Vec2 operator*(Vec2 a, float s){ return {a.x*s,a.y*s}; }
static inline Vec2 operator/(Vec2 a, float s){ return {a.x/s,a.y/s}; }
static inline float dot(Vec2 a, Vec2 b){ return a.x*b.x + a.y*b.y; }
static inline float len(Vec2 a){ return std::sqrt(dot(a,a)); }

// ================== shaders (embedded) ==================
static const char* QUAD_VS = R"(
#version 430
layout(location=0) in vec2 aPos;
void main(){
  gl_Position = vec4(aPos,0,1);
}
)";

static const char* BG_FS = R"(
#version 430
out vec4 FragColor;
uniform vec2 res;
uniform vec2 ballC;
uniform float ballR;
void main(){
  vec2 uv = gl_FragCoord.xy;
  float d = distance(uv, ballC);
  // พื้นหลังดำ, ลูกบอลแดง
  vec3 col = (d <= ballR) ? vec3(0.87,0.22,0.30) : vec3(0.0);
  FragColor = vec4(col,1.0);
}
)";

static const char* LINE_VS = R"(
#version 430
layout(location=0) in vec2 aPos;
layout(location=1) in vec4 aCol;
out vec4 vCol;
uniform vec2 res;
void main(){
  // screen(px) -> NDC
  float x =  2.0*aPos.x/res.x - 1.0;
  float y =  1.0 - 2.0*aPos.y/res.y;
  gl_Position = vec4(x,y,0,1);
  vCol = aCol;
}
)";

static const char* LINE_FS = R"(
#version 430
in vec4 vCol;
out vec4 FragColor;
void main(){ FragColor = vCol; }
)";

// ================== GL helpers ==================
static GLuint makeProgram(const char* vs, const char* fs){
  auto compile = [](GLenum type, const char* src)->GLuint{
    GLuint s = glCreateShader(type);
    glShaderSource(s,1,&src,nullptr);
    glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[4096]; glGetShaderInfoLog(s,4096,nullptr,log);
      std::fprintf(stderr,"[compile] %s\n", log); glDeleteShader(s); return 0; }
    return s;
  };
  GLuint v = compile(GL_VERTEX_SHADER,vs);
  GLuint f = compile(GL_FRAGMENT_SHADER,fs);
  if(!v||!f){ if(v) glDeleteShader(v); if(f) glDeleteShader(f); return 0; }
  GLuint p = glCreateProgram();
  glAttachShader(p,v); glAttachShader(p,f);
  glBindAttribLocation(p,0,"aPos"); // สำหรับ LINE_VS ด้วย
  glBindAttribLocation(p,1,"aCol");
  glLinkProgram(p);
  GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){ char log[4096]; glGetProgramInfoLog(p,4096,nullptr,log);
    std::fprintf(stderr,"[link] %s\n", log); glDeleteProgram(p); p=0; }
  glDeleteShader(v); glDeleteShader(f);
  return p;
}

static GLuint makeFullscreenQuad(){
  float quad[8] = {-1,-1, 1,-1, 1,1, -1,1};
  GLuint vao,vbo; glGenVertexArrays(1,&vao); glBindVertexArray(vao);
  glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
  glBindVertexArray(0);
  return vao;
}

// ================== simulation params ==================
struct Params{
  int   linesN      = 32;        // จำนวนเส้น
  float lineWidth   = 2.0f;      // ความหนา (GL_LINE_WIDTH)
  float sampleStep  = 0.05f;    // ก้าวเวลา (s) ต่อซับสเต็ป
  float sampleGap   = 2.0f;      // ระยะทางขั้นต่ำก่อนจะบันทึกจุดต่อเส้น (px)
  int   maxSamples  = 512;       // ความยาวประวัติของเส้น (จุด)
  float Ux          = 140.0f;    // ลมพื้นฐานซ้าย->ขวา (px/s)
  float spin        = 0.0f;      // สปิน (rad/s), + ตามเข็ม
  float spinAccel   = 2.5f;      // อัตราเร่งสปินจากการกดค้าง (rad/s^2)
  float spinDecay   = 0.35f;     // สัดส่วนหน่วง/วินาที (ประมาณ e^{-decay t})
  float spinMax     = 12.0f;     // เพดานสปิน (rad/s)
  float devBlue     = 40.0f;     // เบี่ยงเบนเท่าไหร่ถึงฟ้าเต็มที่ (px)
  float ballR       = 100.0f;    // รัศมีลูกบอล (px)
  float leftMargin  = -60.0f;    // จุดเริ่ม x ทางซ้าย (px)
  float rightMargin = 60.0f;     // เผื่อขวาแล้วค่อยรีเซ็ต
};

// หนึ่งเส้นลม (หนึ่ง tracer + ประวัติ)
struct Line {
  std::vector<Vec2> pts;   // ประวัติ
  std::vector<Vec2> base;  // เส้นตรงอ้างอิง (y0 คงที่)
  Vec2 p;      // ตำแหน่งปัจจุบัน
  Vec2 pPrev;  // เพื่อเช็คระยะบันทึก
  float y0;    // baseline y
  void reset(float x0, float y){
    pts.clear(); base.clear();
    p = pPrev = {x0,y}; y0 = y;
    pts.push_back(p); base.push_back({x0,y0});
  }
};

// ฟิลด์ความเร็วจาก “ศักย์ไหลรอบกระบอกหมุน” (ชัด Magnus)
// r>=R:  v_r = U(1-R^2/r^2)cosθ
//        v_θ = -U(1+R^2/r^2)sinθ + Γ/(2πr),  Γ = 2π R^2 Ω
static Vec2 velocityField(Vec2 pos, Vec2 ballC, const Params& P){
  float dx = pos.x - ballC.x;
  float dy = pos.y - ballC.y;
  float r2 = dx*dx + dy*dy;
  float R  = P.ballR;
  float R2 = R*R;

  // มุม/ระยะ
  float r = std::sqrt(std::max(r2, 1.0f));
  // ถ้าเผลอหลุดเข้าในกระบอก ให้ยึดขอบไว้ (กันตัวเลขระเบิด)
  if (r < R*1.001f) r = R*1.001f;
  float c = dx/r, s = dy/r; // cosθ, sinθ

  float U = P.Ux;
  float Vr = U * (1.0f - R2/(r*r)) * c;
  float Vt = -U * (1.0f + R2/(r*r)) * s;

  // สปิน -> circulation
  float Gamma = 2.0f * 3.1415926535f * R2 * P.spin;
  Vt += Gamma / (2.0f*3.1415926535f * r);

  // กลับเป็น x,y  : v = (Vr e_r + Vt e_theta)
  // e_r=(c,s), e_t=(-s,c)
  float vx = Vr*c - Vt*s;
  float vy = Vr*s + Vt*c;
  return {vx,vy};
}

int main(){
  if(!glfwInit()) return -1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* win = glfwCreateWindow(1280,720,"Magnus effect (streamlines)",nullptr,nullptr);
  if(!win) return -1;
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);
  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::fprintf(stderr,"glad failed\n"); return -1; }

  // programs
  GLuint progBG   = makeProgram(QUAD_VS, BG_FS);
  GLuint progLine = makeProgram(LINE_VS, LINE_FS);
  GLuint quadVAO  = makeFullscreenQuad();


  // line VAO/VBO (อัปเดตทุกเฟรม)
  GLuint lineVAO, lineVBO;
  glGenVertexArrays(1,&lineVAO); glBindVertexArray(lineVAO);
  glGenBuffers(1,&lineVBO); glBindBuffer(GL_ARRAY_BUFFER,lineVBO);
  glBufferData(GL_ARRAY_BUFFER, 1<<20, nullptr, GL_DYNAMIC_DRAW); // 1MB ชั่วคราว
  glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(float)*6,(void*)0);
  glEnableVertexAttribArray(1); glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(float)*6,(void*)(sizeof(float)*2));
  glBindVertexArray(0);

  Params P;
  int W=1280,H=720;
  glfwGetFramebufferSize(win,&W,&H);
  Vec2 C = { W*0.5f, H*0.5f }; // ศูนย์กลางลูกบอลอัตโนมัติ

  // สร้างเส้นเริ่มต้น
  std::vector<Line> L(P.linesN);
  float gapY = H / float(P.linesN+1);
  for(int i=0;i<P.linesN;i++){
    float y = gapY*(i+1);
    L[i].reset(P.leftMargin, y);
  }

  double tPrev = glfwGetTime();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(P.lineWidth);

  while(!glfwWindowShouldClose(win)){
    // --- time / dt with sub-steps ---
    double tNow = glfwGetTime();
    float dt = float(tNow - tPrev);
    tPrev = tNow;
    dt = std::min(dt, 0.033f); // กันเด้ง

    // --- input ---
    if(glfwGetKey(win, GLFW_KEY_ESCAPE)==GLFW_PRESS) break;
    // Ux
    if(glfwGetKey(win, GLFW_KEY_W)==GLFW_PRESS) P.Ux += 80.0f*dt;
    if(glfwGetKey(win, GLFW_KEY_S)==GLFW_PRESS) P.Ux -= 80.0f*dt;
    P.Ux = std::clamp(P.Ux, 40.0f, 260.0f);
    // spin (กดค้างเร่ง, ปล่อยแล้ว decay)
    int rightHeld = glfwGetKey(win, GLFW_KEY_RIGHT)==GLFW_PRESS;
    int leftHeld  = glfwGetKey(win, GLFW_KEY_LEFT )==GLFW_PRESS;
    if(rightHeld) P.spin += P.spinAccel*dt;
    if(leftHeld)  P.spin -= P.spinAccel*dt;
    if(!rightHeld && !leftHeld){
      P.spin *= std::exp(-P.spinDecay*dt); // exponential decay
    }
    P.spin = std::clamp(P.spin, -P.spinMax, P.spinMax);
    if(glfwGetKey(win, GLFW_KEY_R)==GLFW_PRESS) P.spin = 0.0f;

    // --- window size / center auto ---
    glfwGetFramebufferSize(win,&W,&H);
    C = { W*0.5f, H*0.5f };

    // --- integrate streamlines ---
    // sub-steps เพื่อความเสถียร
    int sub = std::max(1, int(std::ceil(dt / P.sampleStep)));
    float h = dt / sub;

    for(int s=0;s<sub;s++){
      for(int i=0;i<P.linesN;i++){
        Line& line = L[i];

        // ก้าวเวลา (RK2 แบบง่าย)
        Vec2 v0 = velocityField(line.p, C, P);
        Vec2 pm = line.p + v0*(0.5f*h);
        Vec2 v1 = velocityField(pm, C, P);
        line.p  = line.p + v1*(h);

        // เก็บจุดเป็นเส้นเมื่อเดินทางไกลพอ
        if (len(line.p - line.pPrev) >= P.sampleGap){
          if((int)line.pts.size() >= P.maxSamples){
            line.pts.erase(line.pts.begin());
            line.base.erase(line.base.begin());
          }
          line.pts.push_back(line.p);
          line.base.push_back({line.p.x, line.y0});
          line.pPrev = line.p;
        }

        // ออกจากขวา -> รีเซ็ตกลับซ้าย (ไม่ต่อเส้น)
        if(line.p.x > W + P.rightMargin){
          line.reset(P.leftMargin, line.y0);
        }
        // กันไม่ให้หลุดบน-ล่างมากไป (แต่อย่าบังคับ)
        if(line.p.y < -100) line.p.y = -100;
        if(line.p.y > H+100) line.p.y = H+100;
      }
    }

    // ========== draw ==========
    glViewport(0,0,W,H);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    static float spinAngle = 0.0f;
    spinAngle += P.spin * dt;  // ให้ภาพหมุนตามสปินปัจจุบัน
    
    // background + red ball
    glUseProgram(progBG);
    glUniform2f(glGetUniformLocation(progBG,"res"), (float)W,(float)H);
    glUniform2f(glGetUniformLocation(progBG,"ballC"), C.x, C.y);
    glUniform1f(glGetUniformLocation(progBG,"ballR"), P.ballR);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN,0,4);

    // build vertex buffer for all lines (ตำแหน่ง+สี)
    std::vector<float> vbuf; vbuf.reserve(P.linesN * P.maxSamples * 6);
    for(int i=0;i<P.linesN;i++){
      const Line& line = L[i];
      if(line.pts.size()<2) continue;
      for(size_t k=0;k<line.pts.size();k++){
        Vec2 Pcur  = line.pts[k];
        Vec2 Pref  = line.base[k];
        float dev  = std::fabs(Pcur.y - Pref.y);
        float t    = std::clamp(dev / P.devBlue, 0.0f, 1.0f); // 0=ขาว, 1=ฟ้า
        // mix(white, blue)
        float r = 1.0f - 0.2f*t;
        float g = 1.0f - 0.4f*t;
        float b = 1.0f;
        float a = 1.0f;

        vbuf.push_back(Pcur.x); vbuf.push_back(Pcur.y);
        vbuf.push_back(r); vbuf.push_back(g); vbuf.push_back(b); vbuf.push_back(a);
      }
    }

    glUseProgram(progLine);
    glUniform2f(glGetUniformLocation(progLine,"res"), (float)W,(float)H);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER,lineVBO);
    glBufferData(GL_ARRAY_BUFFER, vbuf.size()*sizeof(float), vbuf.data(), GL_DYNAMIC_DRAW);

    // วาดทีละเส้น (ไม่เชื่อมช่วงรีเซ็ต)
    size_t base = 0;
    for(int i=0;i<P.linesN;i++){
      const Line& line = L[i];
      if(line.pts.size()<2) continue;
      glDrawArrays(GL_LINE_STRIP, (GLint)base, (GLint)line.pts.size());
      base += line.pts.size();
    }

    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
