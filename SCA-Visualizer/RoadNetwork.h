#pragma once

#include "glm\glm.hpp"
#include "glad/glad.h"
#include <deque>
#include <vector>
#include <chrono>
#include "MapLayer.h"
#include "Settings.h"
#include "Vertex.h"

static unsigned int attractionPointCount = 10000;
static float segmentLength = 8.0f;			//Generation time significantly increases as this value decreases
static float killDistance = 4.0f;			//Dropping this below 5 increases generation time by orders of magnitude (though that might be to do with being < 0.5 * segment length - any segment that is exactly 0.5 * segment length away will generate a new segment passing through the point that ends up the same distance from the point (out of kill radius))
static int startingSegmentCount = 8;
static float interSegmentAttractionThreshold = 50.0f;
static float segmentConnectionThreshold = 20.0f;

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
	float weightingFactor;
	AttractionPoint(glm::vec2 loc)
	{
		location = loc;
		closest = nullptr;
		weightingFactor = 1.0f;
	}
};

class MajorRoad
{
private:
public:
	std::vector<Segment*> segments;
};

class RoadNetwork
{
private:
	int state;
	int totalConnectors;
	double connectionTime, killTime, closenessNetworkTime;
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
	MapLayer* walkability;
	MapLayer* roadAccess;
	void ConstructAPMesh();
	void ConstructMesh();
	void PickStartingSegments();
	void GenerateClosenessNetwork(std::deque<Segment*>* candidateSegments);
	void InGenerationConnection(std::deque<Segment*>* segmentsAddedInLastRound);
	void PostGenerationConnection();
	void KillPointsNearSegments();
	void AddNewSegmentSet(std::deque<Segment*>* segmentsAddedInLastRound);
public:
	std::vector<glm::vec2> startingLocations;
	RoadNetwork(MapLayer* map, MapLayer* streets);
	~RoadNetwork();
	void SetInitialAttractionPoints();
	void GenerateNetwork();
	void DrawMesh();
	void PrintSummaryStatistics();
	void PrintStateUpdate();
};