#version 330 core
layout (location = 0) in vec2 model;
layout (location = 1) in vec2 UV;

uniform mat4 transform;

out vec2 texCoords;

void main()
{
    gl_Position = transform * vec4(model, 0.0f, 1.0f);
    texCoords = UV;
}