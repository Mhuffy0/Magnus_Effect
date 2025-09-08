#version 430
out vec4 FragColor;
uniform vec2  res;
uniform vec2  ballC;
uniform float ballR;

uniform sampler2D ballTex;
uniform int  useTex;        // 1=ใช้ texture, 0=สีแดง
uniform float spinAngle;    // เราหมุนภาพตามสปิน (เรเดียน)

void main(){
  vec2 p = gl_FragCoord.xy - ballC;
  float d = length(p);

  if(d > ballR){
    FragColor = vec4(0.0,0.0,0.0,1.0); // พื้นหลังดำ
    return;
  }

  // ภายในวงกลม: map -> uv [0,1]^2 และหมุนตาม spinAngle
  vec2 q = p / ballR;                 // -1..1
  float c = cos(spinAngle), s = sin(spinAngle);
  q = mat2(c,-s,s,c) * q;             // หมุนภาพให้หมุนตามลูกบอล
  vec2 uv = q*0.5 + 0.5;              // -> 0..1

  if(useTex==1){
    vec4 tex = texture(ballTex, uv);
    FragColor = vec4(tex.rgb, 1.0);   // โชว์ภาพในลูกบอล
  }else{
    FragColor = vec4(0.87,0.22,0.30,1.0); // สีแดงเดิม
  }
}
