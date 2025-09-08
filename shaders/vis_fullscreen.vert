#version 430
const vec2 verts[3] = vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1,3)); // full-screen triangle
out vec2 fragCoord;
uniform vec2 uResolution;
void main(){
  gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
  // ส่งพิกัดหน้าจอเป็น pixel space ไปที่ frag
  fragCoord = 0.5*(verts[gl_VertexID]+1.0) * uResolution;
}
