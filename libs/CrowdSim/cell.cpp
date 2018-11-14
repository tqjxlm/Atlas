#include "cell.h"
using namespace Ped;

namespace Ped {
	const Tvector pathDirections[8] = {
		Tvector(-0.707f, -0.707f),
		Tvector(.0f, -1.f),
		Tvector(0.707f, -0.707f),
		Tvector(1.f, .0f),
		Tvector(0.707f, 0.707f),
		Tvector(.0f, 1.f),
		Tvector(-0.707f, 0.707f),
		Tvector(-1.f, .0f),
	};

	const Tvector flowDirections[4] = {
		Tvector(.0f, -1.f),
		Tvector(1.f, .0f),
		Tvector(.0f, 1.f),
		Tvector(-1.f, .0f),
	};

	const int cellNeighbourIndex[9][2] = {
		{ -1, -1 },
		{ 0, -1 },
		{ 1, -1 },
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ -1, 1 },
		{ -1, 0 },
		{ 0, 0 }
	};
}

float Cell::width, Cell::height, Cell::area;

float Ped::Cell::maxDensity = 0;

Cell::Cell(float x, float y) :
	_x(x), _y(y), _center{ x + width / 2 , y + height / 2 }
{
}

Ped::Cell::Cell(float x, float y, int idX, int idY) : 
	_x(x), _y(y), _center{ x + width / 2 , y + height / 2 }, _idX(idX), _idY(idY)
{
}

void Ped::Cell::setSize(float w, float h)
{
	width = w;
	height = h;
	area = w * h;
}

bool Ped::Cell::setNextCellInPath(const Cell* destination, Cell* next)
{
	// Assume that path planning is stable, then no need to update
	// when the path has been planned
	if (_nextCellInPath.find(destination) != _nextCellInPath.end())
		return false;

	_nextCellInPath[destination] = next;
	return true;
}