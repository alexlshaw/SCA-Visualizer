#version 330

precision highp float;

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

uniform mat4 projectionViewMatrix;
uniform mat4 modelMatrix;

out Fragment
{
	vec4 color;
} fragment;

void main(void)
{
	fragment.color = color;
	vec4 newPos = modelMatrix * position;
	gl_Position = projectionViewMatrix * newPos;
}