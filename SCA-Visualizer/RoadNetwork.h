#pragma once

#include "glm\glm.hpp"
#include "glad/glad.h"
#include <deque>
#include <vector>
#include "Vertex.h"

static glm::vec4 roadCol = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
static glm::vec4 apCol = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
static glm::vec4 connCol = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
static int attractionPointCount = 1000;
static float segmentLength = 10.0f;			//Generation time significantly increases as this value decreases
static float killDistance = 5.0f;
static int startingSegmentCount = 8;
static float interSegmentAttractionThreshold = 50.0f;
static float segmentConnectionThreshold = 10.0f;

class Segment
{
private:
public:
	Segment(glm::vec2 pos1, glm::vec2 pos2);
	glm::vec4 col;
	glm::vec2 start;
	glm::vec2 end;
	Segment* parent;
	Segment* root;
	std::deque<Segment*> children;
	std::vector<glm::vec2> influenceVectors;
	bool closestFlag; //if the segment was added in the last round, we also check it against other recently added segments
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
	void ConstructMesh();
	void PickStartingSegments();
	void PostGenerationConnection();
public:
	RoadNetwork();
	~RoadNetwork();
	void SetInitialAttractionPoints();
	void GenerateNetwork();
	void DrawMesh();
};