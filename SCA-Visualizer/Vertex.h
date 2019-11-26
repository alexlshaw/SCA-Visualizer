#pragma once

#include "glm\glm.hpp"

struct Vertex
{
	glm::vec4 position;
	glm::vec4 color;
	Vertex(glm::vec4 pos, glm::vec4 col) { position = pos; color = col; }
};

struct TVertex	//Textured Vertex
{
	glm::vec4 position;
	glm::vec2 coord;
	TVertex(glm::vec4 pos, glm::vec2 coord) { position = pos; this->coord = coord; }
};