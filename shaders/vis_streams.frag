#version 430
in vec2 fragCoord;
out vec4 FragColor;

uniform sampler2D uVel;      // RG32F velocity texture (จาก compute)
uniform vec2      uResolution;
uniform float     uTime;

// helper: stripe ขาวบาง ๆ วิ่งขวา
float stripe(float y, float phase){
  float s = fract(y*60.0 + phase); // ความถี่เส้น
  float d = abs(s-0.5);
  return smoothstep(0.5, 0.48, d); // เส้นบาง
}

void main(){
  // สมมติขนาดหน้าจอ = ขนาดซิม (1:1)
  ivec2 ip = ivec2(clamp(fragCoord, vec2(0), uResolution-1.0));
  vec2  u  = texelFetch(uVel, ip, 0).xy;     // อ่านความเร็วจาก compute
  float vmag = length(u);                    // ค่าความแรง (ไว้คูณความสว่าง)

  // ให้เส้นเลื่อนไปทางขวา โดยเร็วตาม u.x (ถ้า u.x < 0 ก็ช้าลง ไม่ย้อน)
  float phase = uTime * max(u.x, 0.0) * 15.0;
  float lines = stripe(fragCoord.y / uResolution.y, phase);

  // พื้นดำ เส้นขาว: base = 0, เส้น+แสงตาม vmag
  float bright = clamp(lines * (0.3 + 3.0*vmag), 0.0, 1.0);
  FragColor = vec4(vec3(bright), 1.0);
}
