#include "RoadNetwork.h"

using namespace std::chrono;

RoadNetwork::RoadNetwork(MapLayer* map, MapLayer* streets)
{
	state = 0;
	totalConnectors = 0;
	connectionTime = 0.0;
	killTime = 0.0;
	closenessNetworkTime = 0.0;
	walkability = map;
	roadAccess = streets;
	SetInitialAttractionPoints();
	PickStartingSegments();
	ConstructAPMesh();
	GenerateNetwork();
	ConstructMesh();
	
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
		base->root = base;
		segments.push_back(base);
		startingLocations.push_back(attractionPoints[idx].location);
	}
}

void RoadNetwork::GenerateClosenessNetwork(std::deque<Segment*>* candidateSegments)
{
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	//generate initial closeness network
	for (auto& point : attractionPoints)
	{
		float closestDistance = point.closest != nullptr ? glm::length(point.location - point.closest->end) : 99999999.9f;	//distance to current closest segment, or an arbitrarily large value
		for (auto& seg : *candidateSegments)
		{
			float currentDistance = glm::length(point.location - seg->end);
			if (currentDistance < closestDistance)
			{
				closestDistance = currentDistance;
				point.closest = seg;
				seg->closestFlag = true;
			}
		}
		if (point.closest != nullptr)
		{
			//make sure we're not working with a 0-length vector
			if (point.location != point.closest->end)
			{
				point.closest->influenceVectors.push_back(glm::normalize(point.location - point.closest->end) * point.weightingFactor);
			}
		}
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	closenessNetworkTime += time_span.count();
}

void RoadNetwork::GenerateNetwork()
{
	//initialise variables
	int remainingAttractionPoints = attractionPointCount;
	int remainingAttractionPointsAtLastIter = remainingAttractionPoints;
	int noProgressCount = 0;
	std::deque<Segment*> segmentsAddedInLastRound;

	//initialise network (we assume that PickStartingSegments() has been called already - since it's just generating our voronoi sites)
	for (auto& seg : segments)
	{
		segmentsAddedInLastRound.push_back(seg);
	}
	//build segments until we have connected just about every point or we have gone for a while without connecting anything new
	while (remainingAttractionPoints > 1 && noProgressCount < 20)
	{
		PrintStateUpdate();
		//regenerate the closeness map from newly added segments
		GenerateClosenessNetwork(&segmentsAddedInLastRound);
		//remove any points where the network has colonised their space
		KillPointsNearSegments();
		//check all of the recently added segments and see if we want to attract them to one another
		//InGenerationConnection(&segmentsAddedInLastRound);
		//Add a new set of segments for this round
		AddNewSegmentSet(&segmentsAddedInLastRound);
		//move the newly created segments into the main collection
		for (auto& n : segmentsAddedInLastRound)
		{
			segments.push_back(n);
		}
		remainingAttractionPoints = attractionPoints.size();
		if (remainingAttractionPoints == remainingAttractionPointsAtLastIter)
		{
			noProgressCount++;
		}
		remainingAttractionPointsAtLastIter = remainingAttractionPoints;
	}
	PrintSummaryStatistics();
	PostGenerationConnection();
}

void RoadNetwork::AddNewSegmentSet(std::deque<Segment*>* segmentsAddedInLastRound)
{
	segmentsAddedInLastRound->clear();
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
			glm::vec2 target = v->end + (sumVector * segmentLength);
			float sLength = segmentLength * roadAccess->AccessibilityBetweenPoints(v->end, target);
			if (sLength > 0.1f)	//only bother to create the segment if it's gonna go anywhere
			{
				Segment* sPrime = new Segment(v->end, v->end + (sumVector * sLength));
				sPrime->parent = v;
				sPrime->root = v->root;
				v->children.push_back(sPrime);
				segmentsAddedInLastRound->push_back(sPrime);
				v->influenceVectors.clear();
			}
		}
	}
}

void RoadNetwork::KillPointsNearSegments()
{
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	std::vector<AttractionPoint> rPoints;
	for (auto& p : attractionPoints)
	{
		//the closeness network has just been generated, so each point should now be tracking its closest segment
		if (glm::length(p.location - p.closest->end) > killDistance)
		{
			rPoints.push_back(p);
		}
	}
	attractionPoints.clear();
	attractionPoints = rPoints;
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	killTime += time_span.count();
}

void RoadNetwork::InGenerationConnection(std::deque<Segment*>* segmentsAddedInLastRound)
{
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	for (auto& seg : *segmentsAddedInLastRound)
	{
		if (!seg->closestFlag)
		{
			//it wasn't close to a point, but it might be close to another segment
			//the trouble with this check is that maybe it should be attracted to another segment that wasn't added in the last round
			for (auto& seg2 : *segmentsAddedInLastRound)
			{
				if (seg->root != seg2->root)
				{

					float dist = glm::length(seg->end - seg2->end);
					if (dist < segmentConnectionThreshold)
					{
						Segment* connector = new Segment(seg->end, seg2->end);
						connector->parent = seg;
						connector->root = seg->root;
						connector->col = connCol;
						totalConnectors++;
						segments.push_back(connector);
					}
					else if (dist < interSegmentAttractionThreshold)
					{
						seg->influenceVectors.push_back(seg2->end - seg->end);
					}
				}
			}
		}
	}
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	connectionTime += time_span.count();
}

void RoadNetwork::PostGenerationConnection()
{
	int totalConnectors = 0;
	//build the list of all end point segments
	std::vector<Segment*> childFree;
	for (auto& seg : segments)
	{
		if (seg->children.size() == 0)
		{
			childFree.push_back(seg);
		}
	}
	for (auto& cf : childFree)
	{
		for (auto& target : childFree)
		{
			//make sure that the segment we are currently looking at still hasn't been connected and that the segments are from different starting networks
			if (cf->root != target->root)	//banning same root is too conservative, but no restriction is too permissive
			{
				float dist = glm::length(cf->end - target->end);
				if (dist < segmentConnectionThreshold)
				{
					Segment* connector = new Segment(cf->end, target->end);
					connector->parent = cf;
					connector->root = cf->root;
					connector->col = connCol;
					cf->children.push_back(connector);
					target->children.push_back(connector);
					totalConnectors++;
					segments.push_back(connector);
				}
			}
		}
	}
	printf("%i total connectors\n", totalConnectors);
}

void RoadNetwork::SetInitialAttractionPoints()
{
	int xRange = 1024;
	int yRange = 1024;
	int x, y;
	for (unsigned int i = 0; i < attractionPointCount; i++)
	{
		x = rand() % xRange;
		y = rand() % yRange;
		//while (!walkability->Walkable(x, y))
		while(roadAccess->RoadScaleFactorFromColor(roadAccess->ColorLookup(x, y)) == IMPASSIBLE)
		{
			x = rand() % xRange;
			y = rand() % yRange;
		}
		AttractionPoint p(glm::vec2((float)x, (float)y));
		p.weightingFactor = roadAccess->RoadScaleFactorFromColor(roadAccess->ColorLookup(x, y));
		attractionPoints.push_back(p);
	}
	printf("%i points generated\n", attractionPoints.size());
}

void RoadNetwork::ConstructAPMesh()
{
	if (attractionPoints.size() > 0)
	{
		int i = 0;
		for (auto& ap : attractionPoints)
		{
			glm::vec2 currentPoint = ap.location;
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
		if (segments.size() > 0)
		{
			for (auto& seg : segments)
			{
				glm::vec2 currentPoint = seg->end;
				Vertex v0 = Vertex(glm::vec4(currentPoint.x - 2.0f, currentPoint.y - 2.0f, 0.0f, 1.0f), siteCol);
				Vertex v1 = Vertex(glm::vec4(currentPoint.x + 2.0f, currentPoint.y - 2.0f, 0.0f, 1.0f), siteCol);
				Vertex v2 = Vertex(glm::vec4(currentPoint.x + 2.0f, currentPoint.y + 2.0f, 0.0f, 1.0f), siteCol);
				Vertex v3 = Vertex(glm::vec4(currentPoint.x - 2.0f, currentPoint.y + 2.0f, 0.0f, 1.0f), siteCol);
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
}

void RoadNetwork::ConstructMesh()
{
	if (segments.size() > 0)
	{
		for (unsigned int i = 0; i < segments.size(); i++)
		{
			Segment s = *segments[i];
			Vertex v0 = Vertex(glm::vec4(s.start.x, s.start.y, 0.0f, 1.0f), s.col);
			Vertex v1 = Vertex(glm::vec4(s.end.x, s.end.y, 0.0f, 1.0f), s.col);
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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
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

void RoadNetwork::PrintSummaryStatistics()
{
	printf("\n--Generation Finished--\n");
	printf("%i total connectors\n", totalConnectors);
	printf("%i total segments\n", segments.size());
	printf("Closeness networks took %f seconds\n", closenessNetworkTime);
	printf("Interconnectivity added %f seconds\n", connectionTime);
	printf("Attraction point removal took %f seconds\n", killTime);
}

void RoadNetwork::PrintStateUpdate()
{
	if (state == 0 && attractionPoints.size() < attractionPointCount / 2)
	{
		printf("%i points remain\n", attractionPoints.size());
		state++;
	}
	else if (state == 1 && attractionPoints.size() < attractionPointCount / 4)
	{
		printf("%i points remain\n", attractionPoints.size());
		state++;
	}
	else if (state == 2 && attractionPoints.size() < attractionPointCount / 8)
	{
		printf("%i points remain\n", attractionPoints.size());
		state++;
	}
}


Segment::Segment(glm::vec2 pos1, glm::vec2 pos2)
{
	this->root = nullptr;
	this->parent = nullptr;
	this->start = pos1;
	this->end = pos2;
	closestFlag = false;
	col = roadCol;
}