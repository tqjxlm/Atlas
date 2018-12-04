#include "grid.h"

#include <iostream>
#include <exception>
#include <limits>

using namespace std;

#include "libpedsim/ped_obstacle.h"
using namespace Ped;

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

// Shortcut to iterate all cells
// The function value should be a lambda expression
#define DO_FOR_ALL_CELLS(function)            \
  for (int i = 0; i < cells.size(); i++)      \
  {                                           \
    for (int j = 0; j < cells[i].size(); j++) \
    {                                         \
      function(cells[i][j]);                  \
    }                                         \
  }                                           \

static const double  DOUBLE_LIMIT      = 0.00001;
static const double  CONVERGENCE_LIMIT = 0.01;

Grid::Grid(double x, double y, double w, double h)
{
	minx = x;
	miny = y;

	Agent::setGrid(this);

	Cell::setSize((float)cellWidth, (float)cellHeight);

	// Generate the grid by row * column
  int  row = 0;

  for (double xx = x; xx < (x + w); xx += cellWidth)
  {
    vector<Cell *>  newRow;
    int             col = 0;

    for (double yy = y; yy < (y + h); yy += cellHeight)
		{
			newRow.push_back(new Cell((float)xx, (float)yy, row, col));
			col++;
		}

		cells.push_back(newRow);
		row++;
	}

	//// Assign neibourghs
  // for (int i = 0; i < cells.size(); i++)
  // {
  // for (int j = 0; j < cells[i].size(); j++)
  // {
  //// Sign in order: left, right, up, down
  // Cell** neighbours = cells[i][j]->getNeighbours();
  // neighbours[0] = j == 0 ? NULL : cells[i][j - 1];
  // neighbours[1] = j == cells[i].size() - 1 ? NULL : cells[i][j + 1];
  // neighbours[2] = i == 0 ? NULL : cells[i - 1][j];
  // neighbours[3] = i == cells.size() - 1 ? NULL : cells[i + 1][j];
  // }
  // }

  // int numCells = cells.size() * cells[0].size();
  // thisSigma = new VectorXd(numCells);
  // nextSigma = new VectorXd(numCells);
  // b = new VectorXd(numCells);
  // Ax = new VectorXd(numCells);
  // press = new VectorXd(numCells);
  // A = new MatrixXd(numCells, numCells);
}

void  Ped::Grid::updateCellAttributes()
{
  DO_FOR_ALL_CELLS([](Cell *cell)
  {
    cell->updateAttribute();
  });
}

void  Ped::Grid::resetCellChangeList()
{
  DO_FOR_ALL_CELLS([](Cell *cell)
  {
    cell->resetChangeList();
  });
}

void  Grid::addAgent(const Agent *agent)
{
  Ped::Tvector  position = agent->getPosition();
  Cell         *cell     = getCellByPosition(position.x, position.y);

	if (cell == NULL)
  {
    throw std::runtime_error("FATAL: Agent goes out of stage!");
  }

	cell->enter(agent);

	gridHash[agent] = cell;
}

void  Grid::moveAgent(const Agent *agent)
{
  Cell *cell = gridHash[agent];

  Ped::Tvector  position = agent->getPosition();
  Cell         *newCell  = getCellByPosition(position.x, position.y);

	if (newCell == NULL)
  {
    throw std::runtime_error("FATAL: Agent goes out of stage!");
  }

	if (cell != newCell)
	{
		cell->leave(agent);
		newCell->enter(agent);
    gridHash[agent]    = newCell;
    cell->changed()    = true;
		newCell->changed() = true;
	}
}

void  Grid::addObstacle(const Tobstacle *obstacle)
{
  double  ax = obstacle->getax();
  double  bx = obstacle->getbx();
  double  ay = obstacle->getay();
  double  by = obstacle->getby();

	// Calculate flop and check for validation
  double  k;

	if (abs(bx - ax) < DOUBLE_LIMIT)
  {
    k = std::numeric_limits<double>::max();
  }
  else
  {
    k = (by - ay) / (bx - ax);
  }

	// Scan for all cells that's affected
	if (abs(k) > 1)
	{
		// Vertically
		// Calculate step size
    double  dy = cellHeight;
    double  dx = dy / k;

		// Start from the top side
    double  x;
    double  y;

		if (ay < by)
		{
			x = ax;
			y = ay;
		}
		else
		{
			x = bx;
			y = by;
		}

    double  lastX = 0;

		// Scan mark cells as inaccessible
		while (y < MAX(ay, by) + dy)
		{
      Cell *cell = getCellByPosition(x, y);

			if (cell)
      {
        cell->attribute().passable = false;
      }

			// A naive anti-aliasing, in order to make line tighter
      if ((lastX != 0) && (abs(cell->x() - lastX) >= cellWidth - DOUBLE_LIMIT))
			{
        Cell *cell = getCellByPosition(x, y - cellHeight);

				if (cell)
        {
          cell->attribute().passable = false;
        }
      }

			lastX = cell->x();

			x += dx;
			y += dy;
		}
	}
	else
	{
		// Horizontally
		// Calculate step size
    double  dx = cellWidth;
    double  dy = dx * k;

		// Start from the left side
    double  x;
    double  y;

		if (ax < bx)
		{
			x = ax;
			y = ay;
		}
		else
		{
			x = bx;
			y = by;
		}

    double  lastY = 0;

		// Scan and mark cells as inaccessible
    while (x < MAX(ax, bx) + dx)
		{
      Cell *cell = getCellByPosition(x, y);

			if (cell)
      {
        cell->attribute().passable = false;
      }

			// A naive anti-aliasing, in order to make line tighter
      if ((lastY != 0) && (abs(cell->y() - lastY) >= cellHeight - DOUBLE_LIMIT))
			{
        Cell *cell = getCellByPosition(x - cellWidth, y);

				if (cell)
        {
          cell->attribute().passable = false;
        }
      }

			lastY = cell->y();

			x += dx;
			y += dy;
		}
	}
}

const Cell::CellAttribute & Grid::getCellAttribute(double x, double y)
{
  Cell *cell = getCellByPosition(x, y);

	if (cell == NULL)
  {
    throw std::runtime_error("Cell index OOR!");
  }

	return cell->attribute();
}

void  Ped::Grid::updateDensityFeild()
{
  DO_FOR_ALL_CELLS([](Ped::Cell *cell)
  {
    cell->updateAttribute();
  });
}

Ped::Grid::~Grid()
{
  // if (thisSigma)
  // {
  // delete thisSigma;
  // delete nextSigma;
  // }
}

// void Ped::Grid::solveUIC()
// {
// DO_FOR_ALL_CELLS([](Ped::Cell* cell) { cell->updateAttribute(); });
//
// do
// {
//// Step 0 - 4
// nextIteration();
//
//// Step 5
// } while ((*nextSigma - *thisSigma).norm() > CONVERGENCE_LIMIT);
//
// }
//
// void Ped::Grid::nextIteration()
// {
//// Step 0
// DO_FOR_ALL_CELLS([](Ped::Cell* cell) { cell->updateGradient(); });
// DO_FOR_ALL_CELLS([](Ped::Cell* cell) { cell->updateDivergence(); });
//
//// Step 1
// int cnt = 0;
// DO_FOR_ALL_CELLS([&] (Ped::Cell* cell) {
// auto attrib = cell->attribute();
// (*thisSigma)[cnt] = attrib.sigma;
// (*b)[cnt] = attrib.divRowV;
// (*Ax)[cnt] = attrib.divRowP / attrib.press;
// (*press)[cnt] = attrib.press;
// cnt++;
// });
//
// *Ax *= -1;
// *b += *thisSigma;
//
//// Step 2
// *A = Ax->asDiagonal().toDenseMatrix();
////*A = Ax->cwiseQuotient(*press);
//
//// Step 3
// solveQP();
//
//// Step 4
// cnt = 0;
// DO_FOR_ALL_CELLS([&](Ped::Cell* cell) {
// cell->updateSigma((*nextSigma)[cnt]);
// cell->updatePressure((*press)[cnt]);
// cnt++;
// });
// }
//
// void Ped::Grid::solveQP()
// {
//
// }
