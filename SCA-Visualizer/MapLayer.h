#pragma once

#include "glad\glad.h"
#include "glm\glm.hpp"
#include <algorithm>
#include <stdio.h>
#include <vector>
#include "Texture.h"
#include "Vertex.h"

const static float HeightScalingFactor = 5.50f;

const static float ROAD_MOTORWAY = 0.5f;
const static float ROAD_MAJOR = 0.4f;
const static float ROAD_MINOR = 0.3f;
const static float ROAD_DRIVEWAY = 0.2f;
const static float ROAD_NONE = 0.1f;


const static int MAPTYPE_HEIGHT = 0;
const static int MAPTYPE_ROADS = 1;

class MapLayer
{
private:
	Texture* tex;
	GLuint vbo, vao, ibo;
	int indexCount;
	std::vector<TVertex> vertices;
	std::vector<int> indices;
	void BuildMesh(int width, int height);
	std::vector<GLubyte> vPixels;
	int mapWidth, mapHeight;
	int mapType;
	glm::vec4 ColorLookup(int x, int y);
	float RoadScaleFactorFromColor(glm::vec4 color);
public:
	MapLayer(const char* path, int width, int height, int type);
	~MapLayer();
	void Draw();
	bool Walkable(int x, int y);
	float MaxmimalSlope(int x, int y);
	float HeightLookup(int x, int y);
	float AccessibilityBetweenPoints(glm::vec2 p1, glm::vec2 p2);	//Returns accessibility value [0...1] ranging from non-accessible to easily accessible
};