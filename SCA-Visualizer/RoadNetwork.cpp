#include "RoadNetwork.h"

RoadNetwork::RoadNetwork()
{
	GenerateNetwork();
	ConstructMesh();
	ConstructAPMesh();
}

RoadNetwork::~RoadNetwork()
{
	for (auto& s : segments)
	{
		delete s;
	}
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &avbo);
	glDeleteBuffers(1, &aibo);
}

void RoadNetwork::PickStartingSegments()
{
	for (int i = 0; i < startingSegmentCount; i++)
	{
		int idx = rand() % attractionPointCount;
		//starting segments are 0-length
		Segment* base = new Segment(attractionPoints[idx].location, attractionPoints[idx].location);
		base->parent = nullptr;
		segments.push_back(base);
	}
}

void RoadNetwork::GenerateNetwork()
{
	SetInitialAttractionPoints();
	PickStartingSegments();
	
	int remainingAttractionPoints = attractionPointCount;
	int remainingAttractionPointsAtLastIter = remainingAttractionPoints;
	int noProgressCount = 0;
	//generate initial closeness network
	for (auto& point : attractionPoints)
	{
		Segment* closest = nullptr;
		float closestDistance = 99999999.9f;	//arbitrarily large
		for (auto& seg : segments)
		{
			float currentDistance = glm::length(point.location - seg->end);
			if (currentDistance < closestDistance)
			{
				closestDistance = currentDistance;
				closest = seg;
			}
		}
		if (closest != nullptr)
		{
			point.closest = closest;
		}
	}
	std::deque<Segment*> segmentsAddedInLastRound;

	//build segments until we have connected *every* area
	while (remainingAttractionPoints > 1 && noProgressCount < 20)
	{
		for (auto& point : attractionPoints)
		{
			//find the segment closest to the attraction point (all points track the last closest segment, so we only check newly added segments)
			float closestDistance = glm::length(point.location - point.closest->end);
			for (auto& seg : segmentsAddedInLastRound)
			{
				if (glm::length(point.location - seg->end) < closestDistance)
				{
					point.closest = seg;
					closestDistance = glm::length(point.location - point.closest->end);
				}
			}

			//calculate the normalized vector to the node and add to the influence vectors
			if (point.closest != nullptr)
			{
				point.closest->influenceVectors.push_back(glm::normalize(point.location - point.closest->end));
			}
		}
		//for each node that got selected as a closest node
		segmentsAddedInLastRound.clear();
		for (auto& v : segments)
		{
			if (v->influenceVectors.size() > 0)
			{
				glm::vec2 sumVector = glm::vec2(0.0f, 0.0f);
				for (auto& vec : v->influenceVectors)
				{
					sumVector = sumVector + vec;
				}
				//there is a risk we have 2 influence vectors that are opposite one another
				if (glm::length(sumVector) < 1.0f)
				{
					sumVector = v->influenceVectors[0];
				}
				//normalise sv
				sumVector = glm::normalize(sumVector);
				//create and attach a new segment
				Segment* sPrime = new Segment(v->end, v->end + (sumVector * segmentLength));
				sPrime->parent = v;
				v->children.push_back(sPrime);
				segmentsAddedInLastRound.push_back(sPrime);
				v->influenceVectors.clear();
			}
		}
		//move the newly created segments into the main collection
		for (auto& n : segmentsAddedInLastRound)
		{
			segments.push_back(n);
		}
		
		//for each attraction point, determine if there are any segments in the kill distance
		std::vector<AttractionPoint> rPoints;
		for (auto& p : attractionPoints)
		{
			bool killThisPoint = false;
			for (auto& s : segments)
			{
				if (glm::length(p.location - s->end) < killDistance)
				{
					killThisPoint = true;
				}
			}
			if (!killThisPoint)
			{
				rPoints.push_back(p);
			}
			
		}
		attractionPoints.clear();
		attractionPoints = rPoints;
		remainingAttractionPoints = attractionPoints.size();
		if (remainingAttractionPoints == remainingAttractionPointsAtLastIter)
		{
			noProgressCount++;
		}
		remainingAttractionPointsAtLastIter = remainingAttractionPoints;
		printf("Added %i segments, %i total segments, %i rap\n", segmentsAddedInLastRound.size(), segments.size(), remainingAttractionPoints);
	}
}

void RoadNetwork::SetInitialAttractionPoints()
{
	int xRange = 1024;
	int yRange = 768;
	for (int i = 0; i < attractionPointCount; i++)
	{
		float rX = (float)(rand() % xRange);
		float rY = (float)(rand() % yRange);
		attractionPoints.push_back(AttractionPoint(glm::vec2(rX, rY)));
	}
}

void RoadNetwork::ConstructAPMesh()
{
	int i = 0;
	for (auto iter = attractionPoints.begin(); iter != attractionPoints.end(); ++iter)
	{
		glm::vec2 currentPoint = (*iter).location;
		Vertex v0 = Vertex(glm::vec4(currentPoint.x, currentPoint.y, 0.0f, 1.0f), apCol);
		Vertex v1 = Vertex(glm::vec4(currentPoint.x + 1.0f, currentPoint.y, 0.0f, 1.0f), apCol);
		Vertex v2 = Vertex(glm::vec4(currentPoint.x + 1.0f, currentPoint.y + 1.0f, 0.0f, 1.0f), apCol);
		Vertex v3 = Vertex(glm::vec4(currentPoint.x, currentPoint.y + 1.0f, 0.0f, 1.0f), apCol);
		APVertices.push_back(v0);
		APVertices.push_back(v1);
		APVertices.push_back(v2);
		APVertices.push_back(v3);
		APIndices.push_back(i * 4);
		APIndices.push_back(i * 4 + 1);
		APIndices.push_back(i * 4 + 2);
		APIndices.push_back(i * 4);
		APIndices.push_back(i * 4 + 2);
		APIndices.push_back(i * 4 + 3);
		i++;
	}
	APindexCount = APIndices.size();
	glGenVertexArrays(1, &avao);
	glGenBuffers(1, &avbo);
	glBindVertexArray(avao);
	glBindBuffer(GL_ARRAY_BUFFER, avbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * APVertices.size(), &APVertices[0], GL_STATIC_DRAW);
	//position
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)0);
	glEnableVertexAttribArray(0);
	//color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)16);
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &aibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * APIndices.size(), &APIndices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RoadNetwork::ConstructMesh()
{
	for (unsigned int i = 0; i < segments.size(); i++)
	{
		Segment s = *segments[i];
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
	//the main mesh
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//the AP mesh
	glBindVertexArray(avao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aibo);
	glDrawElements(GL_TRIANGLES, APindexCount, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


Segment::Segment(glm::vec2 pos1, glm::vec2 pos2)
{
	this->parent = nullptr;
	this->start = pos1;
	this->end = pos2;
}