#version 460 core 

#extension GL_ARB_bindless_texture : require
#extension GL_EXT_shader_image_load_formatted : require

#include<..\..\..\resources\shaders\include\CommonTypes.glsl>
#include<..\..\..\resources\shaders\include\StaticUniformBuffers.glsl>

layout (binding = 0)uniform sampler2D texSampler; 
layout (location = 0)out vec4 FragColor;

layout(location = 0)in vec2 oTex;

void main(void) 
{
    vec4 color = texture(texSampler, oTex);
    FragColor = color;
};