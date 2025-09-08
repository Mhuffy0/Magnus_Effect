#version 430
out vec4 FragColor;
uniform vec2  res;
uniform vec2  circle;
uniform float radius;

void main(){
  vec2 p = gl_FragCoord.xy;
  float d = length(p - circle) - radius;
  if (d > 0.5) discard;
  FragColor = vec4(0.90, 0.10, 0.12, 1.0);   // แดงทึบ
}
