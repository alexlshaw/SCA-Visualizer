#pragma once

#include "glm\glm.hpp"

struct Vertex
{
	glm::vec4 position;
	glm::vec4 color;
	Vertex(glm::vec4 pos, glm::vec4 col) { position = pos; color = col; }
};