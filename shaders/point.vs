#version 430
layout(location=0) in vec2 aPosPx;   // ตำแหน่งอนุภาค หน่วย "พิกเซล"
uniform vec2 res;
void main(){
  vec2 ndc = (aPosPx / res) * 2.0 - 1.0;
  ndc.y = -ndc.y;
  gl_Position = vec4(ndc,0,1);
  gl_PointSize = 2.0;                 // ขนาดจุด = ความหนาเส้น (ปรับได้)
}
