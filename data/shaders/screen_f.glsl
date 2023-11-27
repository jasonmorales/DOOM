#version 330 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D current;

void main()
{
	color = texture(current, TexCoords);
}
