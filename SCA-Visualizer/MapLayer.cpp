#include "MapLayer.h"

MapLayer::MapLayer(const char* path, int width, int height, int type)
{
	mapType = type;
	mapWidth = width;
	mapHeight = height;
	//load the texture data
	tex = new Texture();
	int nWidth, nHeight, nComponents;
	GLenum eFormat;
	GLbyte* pixels = readTGABits(path, &nWidth, &nHeight, &nComponents, &eFormat);
	vPixels = std::vector<GLubyte>(pixels, pixels + (nWidth * nHeight * 4));
	tex->loadFromPixels(vPixels, nWidth, nHeight);
	free(pixels);
	//create the mesh	(might extract this out into a standalone mesh servicing multiple layers if I need to)
	BuildMesh(width, height);


	
	/*float westAccessibility = AccessibilityBetweenPoints(glm::vec2(55.f, 690.f), glm::vec2(55.f, 697.f));	//test accessibility value for very flat land
	printf("West ac is %f\n", westAccessibility);
	float mountAccessibility = AccessibilityBetweenPoints(glm::vec2(676.f, 482.f), glm::vec2(697.f, 478.f));	//test accessibility value for steep slope
	printf("Mount ac is %f\n", mountAccessibility);*/
}

void MapLayer::BuildMesh(int width, int height)
{
	float fW = (float)width;
	float fH = (float)height;
	TVertex v0 = TVertex(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f));
	TVertex v1 = TVertex(glm::vec4(fW, 0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f));
	TVertex v2 = TVertex(glm::vec4(fW, fH, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f));
	TVertex v3 = TVertex(glm::vec4(0.0f, fH, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f));
	vertices.push_back(v0);
	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(0);
	indices.push_back(2);
	indices.push_back(3);
	indexCount = indices.size();
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TVertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	//position
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(TVertex), (const GLvoid*)0);
	glEnableVertexAttribArray(0);
	//color
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TVertex), (const GLvoid*)16);
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MapLayer::Draw()
{
	glActiveTexture(GL_TEXTURE0);
	tex->use();
	//the mesh
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool MapLayer::Walkable(int x, int y)
{
	int pidx = x + 1024 * y;
	return vPixels[4 * pidx + 3] > 0;
}

float MapLayer::MaxmimalSlope(int x, int y)
{
	//detemine index of relevant pixels
	int pidx = x + mapWidth * y;
	int left = x > 0 ? pidx - 1 : pidx;
	int right = x < mapWidth - 1 ? pidx + 1 : pidx;
	int bot = y > 0 ? pidx - mapWidth : pidx;
	int top = y < mapHeight - 1 ? y + mapWidth : y;

	float m = (float)abs(vPixels[4 * pidx + 3] - vPixels[4 * left + 3]);
	m = std::max(m, (float)abs(vPixels[4 * pidx + 3] - vPixels[4 * right + 3]));
	m = std::max(m, (float)abs(vPixels[4 * pidx + 3] - vPixels[4 * top + 3]));
	m = std::max(m, (float)abs(vPixels[4 * pidx + 3] - vPixels[4 * bot + 3]));

	return m;
}

float MapLayer::HeightLookup(int x, int y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= mapWidth) x = mapWidth - 1;
	if (y >= mapHeight) y = mapHeight - 1;
	int pidx = x + mapWidth * y;
	int r = vPixels[4 * pidx];
	int g = vPixels[4 * pidx + 1];
	int b = vPixels[4 * pidx + 2];
	return ((float)(r + g + b)) / HeightScalingFactor;
}

glm::vec4 MapLayer::ColorLookup(int x, int y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= mapWidth) x = mapWidth - 1;
	if (y >= mapHeight) y = mapHeight - 1;
	int pidx = x + mapWidth * y;
	int r = vPixels[4 * pidx];
	int g = vPixels[4 * pidx + 1];
	int b = vPixels[4 * pidx + 2];
	int a = vPixels[4 * pidx + 3];
	return glm::vec4(r, g, b, a);
}

float MapLayer::RoadScaleFactorFromColor(glm::vec4 color)
{
	if (color.r > 220 && color.g > 220 && color.b > 220)
	{
		return ROAD_NONE;
	}
	else if (color.r < 100 && color.g > 220 && color.b > 220)
	{
		return ROAD_MOTORWAY;
	}
	else if (color.g > 100 && color.g > color.r && color.g > color.b)
	{
		return ROAD_MAJOR;
	}
	else if (color.r > 200 && color.g < 100 && color.b < 100)
	{
		return ROAD_MINOR;
	}
	else if (color.b > 200 && color.b > color.g && color.b > color.r)
	{
		return ROAD_DRIVEWAY;
	}
	else if (color.r > 190 && color.b > 140 && color.g > 160 && color.r < 210 && color.b < 160 && color.g < 190)
	{
		return IMPASSIBLE;
	}
	else
	{
		return ROAD_NONE;
	}
}

float MapLayer::AccessibilityBetweenPoints(glm::vec2 p1, glm::vec2 p2)
{
	//heightmap based accessibility
	//float rise = fabs(HeightLookup((int)p1.x, (int)p1.y) - HeightLookup((int)p2.x, (int)p2.y));
	//float run = glm::length(p1 - p2);
	//glm::vec2 rvec = p1 - p2;
	//return fmax(0.0f, 1.0f - (rise / run));


	//roadway based accessibility
	float startRoad = RoadScaleFactorFromColor(ColorLookup((int)p1.x, (int)p1.y));
	float endRoad = RoadScaleFactorFromColor(ColorLookup((int)p2.x, (int)p2.y));
	return startRoad + endRoad;
}

MapLayer::~MapLayer()
{
	delete tex;
}