#pragma once

#include "glm\glm.hpp"
#include "glad/glad.h"
#include <deque>
#include <vector>
#include "Vertex.h"

static glm::vec4 roadCol = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
static glm::vec4 apCol = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
static int attractionPointCount = 500;
static float segmentLength = 5.0f;
static float killDistance = 10.0f;

class Segment
{
private:
public:
	Segment(glm::vec2 pos1, glm::vec2 pos2);
	glm::vec2 start;
	glm::vec2 end;
	Segment* parent;
	std::deque<Segment*> children;
	std::vector<glm::vec2> influenceVectors;
};

class AttractionPoint
{
private:
public:
	glm::vec2 location;
	Segment* closest;
	AttractionPoint(glm::vec2 loc)
	{
		location = loc;
		closest = nullptr;
	}
};

class RoadNetwork
{
private:
	GLuint vbo, vao, ibo;	//buffer identifiers for primary mesh
	GLuint avbo, avao, aibo;	//buffer identifiers for AP mesh
	int indexCount;
	int APindexCount;
	std::deque<Segment*> segments;
	std::vector<Vertex> vertices;
	std::vector<int> indices;
	std::vector<Vertex> APVertices;
	std::vector<int> APIndices;
	std::vector<AttractionPoint> attractionPoints;
	void ConstructAPMesh();
public:
	RoadNetwork();
	~RoadNetwork();
	void SetInitialAttractionPoints();
	void GenerateNetwork();
	void ConstructMesh();
	void DrawMesh();
};