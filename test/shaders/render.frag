#version 460 core

layout (location = 0) in vec2 vcoord;

layout (location = 0) out vec4 fragColor;

layout (binding = 1, rgba32f) uniform image2D img;

void main(){
    fragColor = vec4(imageLoad(img, ivec2(gl_FragCoord.xy)).xyz, 1.f);//vec4(1.f, 0.5f, 0.f, 1.f);
};
