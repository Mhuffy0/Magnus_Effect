#version 430
in vec2 uv;
out vec4 color;
uniform sampler2D uTex;
void main(){
    vec2 u = texture(uTex, uv).xy;
    float m = clamp(length(u) * 20.0, 0.0, 1.0);
    color = vec4(vec3(m), 1.0);
}
