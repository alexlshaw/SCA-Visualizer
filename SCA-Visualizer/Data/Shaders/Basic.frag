#version 330

precision highp float;

in Fragment
{
	vec4 color;
} fragment;

out vec4 finalColor;

void main(void)
{
	finalColor = fragment.color;
}