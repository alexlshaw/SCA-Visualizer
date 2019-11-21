#include "RoadNetwork.h"

RoadNetwork::RoadNetwork()
{
	GenerateNetwork();
	ConstructMesh();
}

void RoadNetwork::GenerateNetwork()
{
	segments.push_back(Segment(glm::vec2(0.0f, 0.0f), glm::vec2(500.0f, 500.0f)));
}

void RoadNetwork::SetInitialPoints()
{

}

void RoadNetwork::Iterate()
{

}

void RoadNetwork::ConstructMesh()
{
	for (int i = 0; i < segments.size(); i++)
	{
		Segment s = segments[i];
		Vertex v0 = Vertex(glm::vec4(s.start.x, s.start.y, 0.0f, 1.0f), roadCol);
		Vertex v1 = Vertex(glm::vec4(s.end.x, s.end.y, 0.0f, 1.0f), roadCol);
		vertices.push_back(v0);
		vertices.push_back(v1);
		indices.push_back(2 * i);
		indices.push_back((2 * i) + 1);
	}
	indexCount = indices.size();
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	//position
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)0);
	glEnableVertexAttribArray(0);
	//color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)16);
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RoadNetwork::DrawMesh()
{
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


Segment::Segment(glm::vec2 pos1, glm::vec2 pos2)
{
	this->start = pos1;
	this->end = pos2;
}