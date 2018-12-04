#pragma once
#include "micropather.h"
using namespace micropather;

#include <math.h>
#include <vector>

namespace Ped
{
class Grid;
class Cell;
}

class PathPlanner:
	public Graph
{
public:
  PathPlanner(const Ped::Grid *grid):
    grid(grid)
	{
		pather = new MicroPather(this, 605532, 8);
	}

  ~PathPlanner()
  {
    delete pather;
  }

  bool           Passable(Ped::Cell *node);

  void           NodeToXY(void *node, int *x, int *y);

  virtual float  LeastCostEstimate(void *nodeStart, void *nodeEnd)
	{
    int  xStart, yStart, xEnd, yEnd;

		NodeToXY(nodeStart, &xStart, &yStart);
		NodeToXY(nodeEnd, &xEnd, &yEnd);

		/* Compute the minimum path cost using distance measurement. It is possible
       to compute the exact minimum path using the fact that you can move only
       on a straight line or on a diagonal, and this will yield a better result.
     */
    int  dx = xStart - xEnd;
    int  dy = yStart - yEnd;

    return (float)sqrt((dx * dx) + (dy * dy));
	}

  virtual void  AdjacentCost(void *node, micropather::MPVector<StateCost> *neighbors);

  virtual void  PrintStateInfo(void *node);

  int           SolvePath(double startX, double startY, double endX, double endY);

  int           SolvePath(Ped::Cell *start, Ped::Cell *end);

  void          clearPath()
  {
    path.resize(0);
  }

  // const std::vector<Ped::PathNode*>& getSolvedPath() { return vectorPath; }
  const MPVector<void *>& getSolvedPath()
  {
    return path;
  }

  void  Reset();

private:
  MPVector<void *>  path;
  MicroPather      *pather;
  const Ped::Grid  *grid;
};
