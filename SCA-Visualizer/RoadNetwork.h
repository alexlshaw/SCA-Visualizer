#pragma once

#include "glm\glm.hpp"
#include "glad/glad.h"
#include <vector>
#include "Vertex.h"

static glm::vec4 roadCol = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

class Segment
{
private:
public:
	Segment(glm::vec2 pos1, glm::vec2 pos2);
	glm::vec2 start;
	glm::vec2 end;
	std::vector<Segment> connectedSegments;
};

class RoadNetwork
{
private:
	GLuint vbo, vao, ibo;
	int indexCount;
	std::vector<Segment> segments;
	std::vector<Vertex> vertices;
	std::vector<int> indices;
public:
	RoadNetwork();
	void SetInitialAttractionPoints();
	void GenerateNetwork();
	void Iterate();
	void ConstructMesh();
	void DrawMesh();
};