#pragma once

#include "glad\glad.h"
#include "glm\glm.hpp"
#include <algorithm>
#include <stdio.h>
#include <vector>
#include "Texture.h"
#include "Vertex.h"

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
public:
	MapLayer(const char* path, int width, int height);
	~MapLayer();
	void Draw();
	bool Walkable(int x, int y);
	float MaxmimalSlope(int x, int y);
	float HeightLookup(int x, int y);
};