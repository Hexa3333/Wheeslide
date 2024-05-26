#version 330 core
out vec4 FragColor;

in vec2 texCoords;
uniform sampler2D sprite;

void main()
{
    vec4 color = texture(sprite, texCoords);
    if(color.a <= 0)
    {
        discard;
    }
    FragColor = color;
}