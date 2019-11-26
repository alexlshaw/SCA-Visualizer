#include "MapLayer.h"

MapLayer::MapLayer(const char* path, int width, int height)
{

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

MapLayer::~MapLayer()
{
	delete tex;
}