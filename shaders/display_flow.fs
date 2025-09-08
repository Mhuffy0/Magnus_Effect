// shaders/display_flow.fs
#version 430
out vec4 o;
uniform vec2 res, circle;  // พิกเซล
uniform float radius;
uniform float omega;       // rad/s (+ = counter-clockwise)

float sdCircle(vec2 p, vec2 c, float r){ return length(p-c) - r; }

void main(){
  vec2 uv = gl_FragCoord.xy;
  // พื้นหลังดำ
  vec3 col = vec3(0.0);

  // วาดลูกบอลแดง
  float d = sdCircle(uv, circle, radius);
  if(d < 0.0){
    col = vec3(0.84, 0.18, 0.22); // red
    // วาดลูกศร 2 อันให้เห็นทิศหมุน
    // สร้างพิกัดเชิงขั้วในลูกบอล
    vec2 q = uv - circle;
    float r = length(q) / radius;
    float a = atan(q.y, q.x);     // -pi..pi

    // สองแถบโค้งบาง ๆ + หัวลูกศร
    for(int k=0;k<2;++k){
      float a0 = (k==0? 1.2 : -2.0);            // มุมกึ่งกลางของโค้ง
      float band = 0.06;                         // ความหนา
      float curve = abs(a - a0) - 0.6;           // ความโค้ง
      float on = smoothstep(0.02,0.0, abs(curve)) * smoothstep(0.85,0.75,r);
      col = mix(col, vec3(0.05), on);            // สีดำเข้ม
    }
  }

  o = vec4(col,1.0);
}
