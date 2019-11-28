#include "Voronoi.h"

Voronoi::Voronoi(float minDistanceBetweenSites, std::vector<glm::vec2> points, float minX, float maxX, float minY, float maxY)
{
	//init variables with default values
	ELhashsize = 0;
	ELleftend = nullptr;
	ELrightEnd = nullptr;
	bottomsite = nullptr;
	PQcount = 0;
	PQhashsize = 0;
	PQmin = 0;
	borderMaxX = maxX, borderMinX = minX;
	borderMaxY = maxY, borderMinY = minY;
	deltax = 0.0f, deltay = 0.0f;
	nedges = 0, nsites = 0, nvertices = 0, sqrt_nsites = 0;
	xmax = 0, xmin = 0;
	ymax = 0, ymin = 0;
	vao = -1, vbo = -1, ibo = -1, indexCount = 0;

	siteIdx = 0;
	this->minDistanceBetweenSites = minDistanceBetweenSites;
	sort(points);
	voronoi_bd();
}

Voronoi::~Voronoi()
{
	//delete all voronoi objects created by the class
	for (unsigned int i = 0; i < sites.size(); i++)
	{
		if (sites[i] != nullptr)
		{
			delete sites[i];
		}
	}
	for (unsigned int i = 0; i < PQHash.size(); i++)
	{
		if (PQHash[i] != nullptr)
		{
			delete PQHash[i];
			PQHash[i] = nullptr;
		}
	}
	for (unsigned int i = 0; i < ELhash.size(); i++)
	{
		for (unsigned int j = i + 1; j < ELhash.size(); j++)
		{
			if (ELhash[j] == ELhash[i])
			{
				ELhash[j] = nullptr;
			}
		}
		if (ELhash[i] != nullptr)
		{
			delete ELhash[i];
		}
	}
	for (unsigned int i = 0; i < allEdges.size(); i++)
	{
		if (allEdges[i] != nullptr)
		{
			delete allEdges[i];
		}
	}
	for (unsigned int i = 0; i < corners.size(); i++)
	{
		if (corners[i] != nullptr)
		{
			delete corners[i];
		}
	}
}

void Voronoi::sort(std::vector<glm::vec2> points)
{
	nsites = points.size();
	nvertices = 0;
	nedges = 0;
	float sn = (float)(nsites + 4);
	sqrt_nsites = (int)(sqrt(sn));
	sortnode(points);
}

void Voronoi::sortnode(std::vector<glm::vec2> points)
{
	nsites = points.size();
	sites.reserve(nsites);
	xmin = points[0].x;
	ymin = points[0].y;
	xmax = points[0].x;
	ymax = points[0].y;

	for (int i = 0; i < nsites; i++)
	{
		sites.push_back(new Site);		//have to trust that the back index is i
		sites[i]->coord = points[i];
		sites[i]->sitenbr = i;

		if (points[i].x < xmin)
		{
			xmin = points[i].x;
		}
		else if (points[i].x > xmax)
		{
			xmax = points[i].x;
		}
		if (points[i].y < ymin)
		{
			ymin = points[i].y;
		}
		else if (points[i].y > ymax)
		{
			ymax = points[i].y;
		}
	}

	std::sort(sites.begin(), sites.end(), [](Site* s1, Site* s2)
	{
		if (s1->coord.y < s2->coord.y)
		{
			return true;
		}
		else if (s1->coord.y > s2->coord.y)
		{
			return false;
		}
		if (s1->coord.x < s2->coord.x)
		{
			return true;
		}
		else if (s1->coord.x > s2->coord.x)
		{
			return false;
		}
		return false;
	});
	deltax = xmax - xmin;
	deltay = ymax - ymin;
}

Site* Voronoi::nextOne()
{
	if (siteIdx < nsites)
	{
		Site* s = sites[siteIdx];
		siteIdx++;
		return s;
	}
	return nullptr;
}

Edge* Voronoi::bisect(Site* s1, Site* s2)
{
	float dx, dy, adx, ady;
	Edge* newedge = new Edge;
	newedge->reg.push_back(s1);	//newedge->reg[0] = s1;
	newedge->reg.push_back(s2); //newedge->reg[1] = s2;
	newedge->ep.push_back(nullptr);//newedge->ep[0] = nullptr;
	newedge->ep.push_back(nullptr);//newedge->ep[1] = nullptr;

	dx = s2->coord.x - s1->coord.x;
	dy = s2->coord.y - s1->coord.y;

	adx = dx > 0.0f ? dx : -dx;
	ady = dy > 0.0f ? dy : -dy;
	newedge->c = (float)(s1->coord.x * dx + s1->coord.y * dy + (dx * dx + dy * dy) * 0.5f);

	if (adx > ady)
	{
		newedge->a = 1.0f;
		newedge->b = dy / dx;
		newedge->c /= dx;
	}
	else
	{
		newedge->a = dx / dy;
		newedge->b = 1.0f;
		newedge->c /= dy;
	}

	newedge->edgenbr = nedges;
	nedges++;
	return newedge;
}

void Voronoi::makevertex(Site* v)
{
	v->sitenbr = nvertices;
	nvertices++;
}

bool Voronoi::PQinitialise()
{
	PQcount = 0;
	PQmin = 0;
	PQhashsize = 4 * sqrt_nsites;
	PQHash.reserve(PQhashsize);
	for (int i = 0; i < PQhashsize; i++)
	{
		HalfEdge* newEdge = new HalfEdge;
		newEdge->deleted = false;
		newEdge->PQnext = nullptr;
		PQHash.push_back(newEdge);
	}
	return true;
}

int Voronoi::PQbucket(HalfEdge* he)
{
	int bucket;
	bucket = (int)((he->ystar - ymin) / deltay * PQhashsize);
	if (bucket < 0)
	{
		bucket = 0;
	}
	if (bucket >= PQhashsize)
	{
		bucket = PQhashsize - 1;
	}
	if (bucket < PQmin)
	{
		PQmin = bucket;
	}
	return bucket;
}

//push the halfedge into the ordered linked list of vertices
void Voronoi::PQinsert(HalfEdge* he, Site* v, float offset)
{
	HalfEdge* last = nullptr, * next = nullptr;
	he->vertex = v;
	he->ystar = v->coord.y + offset;
	last = PQHash[PQbucket(he)];
	while ((next = last->PQnext) != nullptr &&
		(he->ystar > next->ystar || (he->ystar == next->ystar && v->coord.x > next->vertex->coord.x)))
	{
		last = next;
	}
	he->PQnext = last->PQnext;
	last->PQnext = he;
	PQcount++;
}

//remove the halfedge from the list of vertices
void Voronoi::PQDelete(HalfEdge* he)
{
	HalfEdge* last = nullptr;
	if (he->vertex != nullptr)
	{
		last = PQHash[PQbucket(he)];
		while (last->PQnext != he)
		{
			last = last->PQnext;
		}
		last->PQnext = he->PQnext;
		PQcount--;
		he->vertex = nullptr;
	}
}

bool Voronoi::PQEmpty()
{
	return PQcount == 0;
}

glm::vec2 Voronoi::PQ_min()
{
	while (PQHash[PQmin]->PQnext == nullptr)
	{
		PQmin++;
	}
	return glm::vec2(PQHash[PQmin]->PQnext->vertex->coord.x, PQHash[PQmin]->PQnext->ystar);
}

HalfEdge* Voronoi::PQextractmin()
{
	HalfEdge* curr = nullptr;
	curr = PQHash[PQmin]->PQnext;
	PQHash[PQmin]->PQnext = curr->PQnext;
	PQcount--;
	return curr;
}

HalfEdge* Voronoi::HEcreate(Edge* e, int pm)
{
	HalfEdge* answer = new HalfEdge;
	answer->ELedge = e;
	answer->ELpm = pm;
	answer->PQnext = nullptr;
	answer->vertex = nullptr;
	answer->deleted = false;
	answer->ystar = 0.0f;
	return answer;
}

bool Voronoi::ELinitialise()
{
	ELhashsize = 2 * sqrt_nsites;
	//fill the hash table with nullptrs
	ELhash.reserve(ELhashsize);
	for (int i = 0; i < ELhashsize; i++)
	{
		ELhash.push_back(nullptr);
	}
	ELleftend = HEcreate(nullptr, 0);
	ELrightEnd = HEcreate(nullptr, 0);
	ELleftend->ELleft = nullptr;
	ELleftend->ELright = ELrightEnd;
	ELrightEnd->ELleft = ELleftend;
	ELrightEnd->ELright = nullptr;
	ELhash[0] = ELleftend;
	ELhash[ELhashsize - 1] = ELrightEnd;
	return true;
}

HalfEdge* Voronoi::ELright(HalfEdge* he)
{
	return he->ELright;
}

HalfEdge* Voronoi::ELleft(HalfEdge* he)
{
	return he->ELleft;
}

Site* Voronoi::leftreg(HalfEdge* he)
{
	if (he->ELedge == nullptr)
	{
		return bottomsite;
	}
	return he->ELpm == LE ? he->ELedge->reg[LE] : he->ELedge->reg[RE];
}

void Voronoi::ELinsert(HalfEdge* lb, HalfEdge* newHe)
{
	newHe->ELleft = lb;
	newHe->ELright = lb->ELright;
	(lb->ELright)->ELleft = newHe;
	lb->ELright = newHe;
}

//the delete function cannot reclaim the node, since pointers from the hash table may be present
void Voronoi::ELdelete(HalfEdge* he)
{
	(he->ELleft)->ELright = he->ELright;
	(he->ELright)->ELleft = he->ELleft;
	he->deleted = true;
}

HalfEdge* Voronoi::ELgethash(int b)
{
	HalfEdge* he = nullptr;
	if (b < 0 || b >= ELhashsize)
	{
		return nullptr;
	}
	he = ELhash[b];
	if (he == nullptr || !he->deleted)
	{
		return he;
	}
	//if we make it this far, hash table points to deleted half edge. remove from the hash table. NOTE: calling delete on this item causes an error - I assume it is referenced in multiple places
	ELhash[b] = nullptr;
	return nullptr;
}

HalfEdge* Voronoi::ELleftbnd(glm::vec2 p)
{
	int bucket;
	HalfEdge* he = nullptr;

	//use hash table to get close to desired halfedge
	//use the hash function to find the place in the hash map that this halfedge should be
	bucket = (int)((p.x - xmin) / deltax * ELhashsize);

	//make sure the bucket position is within the range of the hash array
	if (bucket < 0)
	{
		bucket = 0;
	}
	if (bucket >= ELhashsize)
	{
		bucket = ELhashsize - 1;
	}

	he = ELgethash(bucket);

	//if the halfedge isn't found, search backwards and fowards in the hash map for the first non-null entry
	if (he == nullptr)
	{
		for (int i = 1; i < ELhashsize; i++)
		{
			if ((he = ELgethash(bucket - i)) != nullptr)
			{
				break;
			}
			if ((he = ELgethash(bucket + i)) != nullptr)
			{
				break;
			}
		}
	}

	//now search linear list of halfedges for the correct one
	if (he == ELleftend || (he != ELrightEnd && right_of(he, p)))
	{
		//keep going right on the list until either the end is reached, or
		//you find the 1st edge which the point isn't to the right of
		do
		{
			if (he != nullptr)
			{
				he = he->ELright;
			}
		} while (he != ELrightEnd && right_of(he, p));
		if (he != nullptr)
		{
			he = he->ELleft;
		}
	}
	else
	{
		//if the point is to the left of the halfedge, the search left for the HE just to the left of the point
		do
		{
			if (he != nullptr)
			{
				he = he->ELleft;
			}
			
		} while (he != ELleftend && !right_of(he, p));
	}
	if (bucket > 0 && bucket < ELhashsize - 1)
	{
		ELhash[bucket] = he;
	}
	return he;
}

void Voronoi::pushGraphEdge(Site* leftSite, Site* rightSite, glm::vec2 p1, glm::vec2 p2)
{
	GraphEdge* newEdge = new GraphEdge;
	newEdge->p1 = p1;
	newEdge->p2 = p2;
	newEdge->site1 = leftSite;
	newEdge->site2 = rightSite;

	//add corners
	newEdge->c1 = addCorner(leftSite, rightSite, p1);
	newEdge->c2 = addCorner(leftSite, rightSite, p2);

	allEdges.push_back(newEdge);

	//add references to the sites
	leftSite->adjacentSites.push_back(rightSite);
	leftSite->edges.push_back(newEdge);
	rightSite->adjacentSites.push_back(leftSite);
	rightSite->edges.push_back(newEdge);
}

//I suspect the purpose of this function is to take the arbitrarily long edges
//generated so far, and trim them to fit in the region we have defined
//possibly also to trim them so that they stop at intersections, but it doesn't really look like it
void Voronoi::clip_line(Edge* e)
{
	float pxmin, pxmax, pymin, pymax;
	Site* s1 = nullptr, * s2 = nullptr;

	float x1 = e->reg[0]->coord.x;
	float y1 = e->reg[0]->coord.y;
	float x2 = e->reg[1]->coord.x;
	float y2 = e->reg[1]->coord.y;
	float x = x2 - x1;
	float y = y2 - y1;

	//if the distance between the two points this line was created from is less than root2 (actually just < 0.1), then ignore it
	if (glm::length(glm::vec2(x, y)) < minDistanceBetweenSites)
	{
		return;
	}
	pxmin = borderMinX;
	pymin = borderMinY;
	pxmax = borderMaxX;
	pymax = borderMaxY;

	if (e->a == 1.0f && e->b >= 0.0f)
	{
		s1 = e->ep[1];
		s2 = e->ep[0];
	}
	else
	{
		s1 = e->ep[0];
		s2 = e->ep[1];
	}

	if (e->a == 1.0f)
	{
		y1 = pymin;

		if (s1 != nullptr && s1->coord.y > pymin)
		{
			y1 = s1->coord.y;
		}
		if (y1 > pymax)
		{
			y1 = pymax;
		}
		x1 = e->c - e->b * y1;
		y2 = pymax;

		if (s2 != nullptr && s2->coord.y < pymax)
		{
			y2 = s2->coord.y;
		}
		if (y2 < pymin)
		{
			y2 = pymin;
		}
		x2 = e->c - e->b * y2;
		//if the x-values are outside of the borders, discard
		if (((x1 > pxmax) && (x2 > pxmax)) || ((x1 < pxmin) && (x2 < pxmin)))
		{
			return;
		}

		//check bounds
		if (x1 > pxmax)
		{
			x1 = pxmax;
			y1 = (e->c - x1) / e->b;
		}
		if (x1 < pxmin)
		{
			x1 = pxmin;
			y1 = (e->c - x1) / e->b;
		}
		if (x2 > pxmax)
		{
			x2 = pxmax;
			y2 = (e->c - x2) / e->b;
		}
		if (x2 < pxmin)
		{
			x2 = pxmin;
			y2 = (e->c - x2) / e->b;
		}
	}
	else
	{
		x1 = pxmin;
		if (s1 != nullptr && s1->coord.x > pxmin)
		{
			x1 = s1->coord.x;
		}
		if (x1 > pxmax)
		{
			x1 = pxmax;
		}
		y1 = e->c - e->a * x1;

		x2 = pxmax;
		if (s2 != nullptr && s2->coord.x < pxmax)
		{
			x2 = s2->coord.x;
		}
		if (x2 < pxmin)
		{
			x2 = pxmin;
		}
		y2 = e->c - e->a * x2;

		//if the y-values are outside of the borders, discard
		if (((y1 > pymax) && (y2 > pymax)) || ((y1 < pymin) && (y2 < pymin)))
		{
			return;
		}

		//check bounds
		if (y1 > pymax)
		{
			y1 = pymax;
			x1 = (e->c - y1) / e->a;
		}
		if (y1 < pymin)
		{
			y1 = pymin;
			x1 = (e->c - y1) / e->a;
		}
		if (y2 > pymax)
		{
			y2 = pymax;
			x2 = (e->c - y2) / e->a;
		}
		if (y2 < pymin)
		{
			y2 = pymin;
			x2 = (e->c - y2) / e->a;
		}
	}
	pushGraphEdge(e->reg[0], e->reg[1], glm::vec2(x1, y1), glm::vec2(x2, y2));
}

void Voronoi::endpoint(Edge* e, int lr, Site* s)
{
	e->ep[lr] = s;
	if (e->ep[RE - lr] == nullptr)
	{
		return;
	}
	clip_line(e);
}

//returns true if p is to right of halfedge e
bool Voronoi::right_of(HalfEdge* el, glm::vec2 p)
{
	Edge* e = nullptr;
	Site* topsite = nullptr;
	bool right_of_site;
	bool above, fast;
	float dxp, dyp, dxs, t1, t2, t3, yl;
	e = el->ELedge;
	topsite = e->reg[1];

	right_of_site = p.x > topsite->coord.x;

	if (right_of_site && el->ELpm == LE)
	{
		return true;
	}
	if (!right_of_site && el->ELpm == RE)
	{
		return false;
	}

	if (e->a == 1.0f)
	{
		dxp = p.x - topsite->coord.x;
		dyp = p.y - topsite->coord.y;
		fast = false;

		if ((!right_of_site && (e->b < 0.0f)) | (right_of_site && (e->b >= 0.0f)))
		{
			above = dyp >= e->b * dxp;
			fast = above;
		}
		else
		{
			above = p.x + p.y * e->b > e->c;
			if (e->b < 0.0f)
			{
				above = !above;
			}
			if (!above)
			{
				fast = true;
			}
		}
		if (!fast)
		{
			dxs = topsite->coord.x - (e->reg[0])->coord.x;
			above = (e->b * (dxp * dxp - dyp * dyp) < dxs * dyp * (1.0f + 2.0f * dxp / dxs + e->b * e->b));

			if (e->b < 0)
			{
				above = !above;
			}
		}
	}
	else // e.b == 1.0f
	{
		yl = e->c - e->a * p.x;
		t1 = p.y - yl;
		t2 = p.x - topsite->coord.x;
		t3 = yl - topsite->coord.y;
		above = t1 * t1 > t2* t2 + t3 * t3;
	}
	return el->ELpm == LE ? above : !above;
}

Site* Voronoi::rightreg(HalfEdge* he)
{
	if (he->ELedge == nullptr)
	{
		return bottomsite;
	}
	return he->ELpm == LE ? he->ELedge->reg[RE] : he->ELedge->reg[LE];
}

float Voronoi::dist(Site* s, Site* t)
{
	return glm::distance(s->coord, t->coord);
}

//Create a new site where the halfedges el1 and el2 intersect
Site* Voronoi::intersect(HalfEdge* el1, HalfEdge* el2)
{
	Edge* e1 = nullptr, * e2 = nullptr, * e = nullptr;
	HalfEdge* el = nullptr;
	float d, xint, yint;
	bool right_of_site;
	Site* v = nullptr;	//vertex

	e1 = el1->ELedge;
	e2 = el2->ELedge;

	if (e1 == nullptr || e2 == nullptr)	//if either edge is null, return nullptr
	{
		return nullptr;
	}

	//if the two edges bisect the same parent, return nullptr
	if (e1->reg[1] == e2->reg[1])
	{
		return nullptr;
	}

	d = e1->a * e2->b - e1->b * e2->a;
	if (-1.0e-10 < d && d < 1.0e-10)	//if d is approximately 0, return nullptr
	{
		return nullptr;
	}

	xint = (e1->c * e2->b - e2->c * e1->b) / d;
	yint = (e2->c * e1->a - e1->c * e2->a) / d;

	if ((e1->reg[1]->coord.y < e2->reg[1]->coord.y) || (e1->reg[1]->coord.y == e2->reg[1]->coord.y && e1->reg[1]->coord.x < e2->reg[1]->coord.x))
	{
		el = el1;
		e = e1;
	}
	else
	{
		el = el2;
		e = e2;
	}

	right_of_site = xint >= e->reg[1]->coord.x;
	if ((right_of_site && el->ELpm == LE) || (!right_of_site && el->ELpm == RE))
	{
		return nullptr;
	}
	// create a new site at the point of intersection - this is a new vector event waiting to happen
	v = new Site();
	v->coord.x = xint;
	v->coord.y = yint;
	return v;
}

bool Voronoi::voronoi_bd()
{
	Site* newsite = nullptr, * bot = nullptr, * top = nullptr, * temp = nullptr, * p = nullptr;
	Site* v = nullptr;
	glm::vec2 newintstar = glm::vec2(0.0f, 0.0f);
	int pm;
	HalfEdge* lbnd = nullptr, * rbnd = nullptr, * llbnd = nullptr, * rrbnd = nullptr, * bisector = nullptr;
	Edge* e = nullptr;

	PQinitialise();
	ELinitialise();

	bottomsite = nextOne();
	newsite = nextOne();

	while (true)
	{
		if (!PQEmpty())
		{
			newintstar = PQ_min();
		}

		// if the lowest site has a smaller y value than the lowest vector intersection,
		// process the site otherwise process the vector intersection
		if (newsite != nullptr && (PQEmpty()
			|| newsite->coord.y < newintstar.y
			|| (newsite->coord.y == newintstar.y
				&& newsite->coord.x < newintstar.x)))
		{
			//new site is smallest - this is a site event
			//get the first halfedge to the LEFT of the new site
			lbnd = ELleftbnd(newsite->coord);
			//get the first halfedge to the RIGHT of the new site
			rbnd = ELright(lbnd);
			//if this halfedge has no edge, bot = bottom site (whatever that is)
			bot = rightreg(lbnd);
			//create a new edge that bisects
			e = bisect(bot, newsite);

			//create a new halfedge, setting its ELpm field to 0
			bisector = HEcreate(e, LE);
			//insert this bisector edge between the left and right vectors in a linked list
			ELinsert(lbnd, bisector);

			//if the new bisector intersects with the left edge, remove the left edge's vertex and put in the new one
			if ((p = intersect(lbnd, bisector)) != nullptr)
			{
				PQDelete(lbnd);
				PQinsert(lbnd, p, dist(p, newsite));
			}
			lbnd = bisector;
			//create a new halfedge, setting its ELpm field to 1
			bisector = HEcreate(e, RE);
			//insert the new HE to the right of the original bisector earlier in the if statement
			ELinsert(lbnd, bisector);

			//if this new bisector intersects with the new halfedge
			if ((p = intersect(bisector, rbnd)) != nullptr)
			{
				PQinsert(bisector, p, dist(p, newsite));	//push the HE into the ordered linked list of vertices
			}
			newsite = nextOne();
		}
		else if (!PQEmpty())	//intersection is smallest - this is a vector event
		{
			//pop the halfedge with the lowest vector off the ordered list of vectors
			lbnd = PQextractmin();
			//get the halfedge to the left of the above HE
			llbnd = ELleft(lbnd);
			//get the halfedge to the right of the above hE
			rbnd = ELright(lbnd);
			//get the halfedge to the right of the HE to the right of the lowest HE
			rrbnd = ELright(rbnd);
			//get the site to the left of the left HE which it bisects
			bot = leftreg(lbnd);
			//get the Site to the right of the right HE which it bisects
			top = rightreg(rbnd);

			v = lbnd->vertex;	//get the vertex that caused the event
			makevertex(v);		//set the vertex number - couldn't do this earlier since we didn't know when it would be processed
			endpoint(lbnd->ELedge, lbnd->ELpm, v);	//set the endpoint of the left HE to this vector
			endpoint(rbnd->ELedge, rbnd->ELpm, v);	//set the endpoint of the right HE to this vector
			ELdelete(lbnd);		//mark the lowest HE for deletion (can't delete yet as there may be pointers to it in the hashmap)
			PQDelete(rbnd);		//remove all vertex events to do with the right HE
			ELdelete(rbnd);		//mark the right HE for deletion
			pm = LE;

			//if the site to the left of the event is higher than the site to the right of it
			//then swap them and set the pm variable to 1
			if (bot->coord.y > top->coord.y)
			{
				temp = bot;
				bot = top;
				top = temp;
				pm = RE;
			}
			e = bisect(bot, top);	//create an edge between the two sites, with formula and line#
			bisector = HEcreate(e, pm);	//create a HE from the Edge e and make it point to that edge with its ELedge field
			ELinsert(llbnd, bisector); //insert the new bisector to the right of the left HE
			endpoint(e, RE - pm, v);	//set one endpoint of the new edge to be the vector point v
										//if the site to the left of this bisector is higher than the right site, then this endpoint
										//is put in position 0, otherwise in position 1

			//if the left HE and the new bisector intersect, then delete the left HE and reinsert it
			if ((p = intersect(llbnd, bisector)) != nullptr)
			{
				PQDelete(llbnd);
				PQinsert(llbnd, p, dist(p, bot));
			}

			//if the right HE and the new bisector intersect, then reinsert it
			if ((p = intersect(bisector, rrbnd)) != nullptr)
			{
				PQinsert(bisector, p, dist(p, bot));
			}
		}
		else
		{
			break;
		}
	}

	//iterate over the linked list of halfedges
	for (lbnd = ELright(ELleftend); lbnd != ELrightEnd; lbnd = ELright(lbnd))
	{
		e = lbnd->ELedge;
		clip_line(e);
	}

	return true;
}

Corner* Voronoi::addCorner(Site* s1, Site* s2, glm::vec2 pos)
{
	//we have a location of a potential corner to be added between these two sites
	//but, either of the sites may already have a corner at that location
	//so we check the sites for the corner
	Corner* c = nullptr;
	bool existsInS1 = false;
	bool existsInS2 = false;
	for (unsigned int i = 0; i < s1->corners.size(); i++)
	{
		//if (s1->corners[i]->pos.x == x && s1->corners[i]->pos.y == y)
		if (glm::distance(pos, s1->corners[i]->pos) < 0.001f)
		{
			//the corner exists in s1
			c = s1->corners[i];
			existsInS1 = true;
		}
	}

	for (unsigned int i = 0; i < s2->corners.size(); i++)
	{
		//if (s2->corners[i]->pos.x == x && s2->corners[i]->pos.y == y)
		if (glm::distance(pos, s2->corners[i]->pos) < 0.001f)
		{
			//the corner exists in s1
			c = s2->corners[i];
			existsInS2 = true;
		}
	}

	if (c != nullptr)
	{
		//at least one of the sites already had it
		if (!existsInS1)
		{
			s1->corners.push_back(c);
			c->touches.push_back(s1);
		}
		else if (!existsInS2)
		{
			s2->corners.push_back(c);
			c->touches.push_back(s2);
		}
	}
	else
	{
		//corner does not exist yet, create and add it
		c = new Corner;
		c->pos = pos;// glm::vec2(x, y);
		s1->corners.push_back(c);
		s2->corners.push_back(c);
		c->touches.push_back(s1);
		c->touches.push_back(s2);
		corners.push_back(c);
	}
	return c;
}

void Voronoi::BuildMesh()
{
	for (auto& site : sites)
	{
		for (auto& e : site->edges)
		{
			Vertex v0 = Vertex(glm::vec4(e->p1, 0.0f, 1.0f), vCol);
			Vertex v1 = Vertex(glm::vec4(e->p2, 0.0f, 1.0f), vCol);
			vertices.push_back(v0);
			vertices.push_back(v1);
			indices.push_back(indexCount);
			indexCount++;
			indices.push_back(indexCount);
			indexCount++;
		}
	}
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

void Voronoi::DrawDiagram()
{
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, (void*)0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}