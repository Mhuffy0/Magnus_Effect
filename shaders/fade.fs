#version 430
out vec4 FragColor;
uniform float fade;                   // 0..1 ยิ่งมากยิ่งเฟดเร็ว
void main(){ FragColor = vec4(0.0,0.0,0.0, fade); }
