#version 330

precision highp float;

layout (location = 0) in vec4 position;
layout (location = 1) in vec2 coords;

uniform mat4 projectionViewMatrix;
uniform mat4 modelMatrix;

out Fragment
{
	vec2 coords;
} fragment;

void main(void)
{
	fragment.coords = coords;
	vec4 newPos = modelMatrix * position;
	gl_Position = projectionViewMatrix * newPos;
}