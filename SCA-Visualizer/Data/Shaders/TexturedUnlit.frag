#version 330

precision highp float;

uniform sampler2D tex;

in Fragment
{
	vec2 coords;
} fragment;

out vec4 finalColor;

void main(void)
{
	finalColor = texture(tex, fragment.coords);
}