#include "PathPlanner.h"

#include "grid.h"

inline bool PathPlanner::Passable(Ped::Cell* node)
{
	if (node == NULL)
		return false;
	return node->attribute().passable;
}

void PathPlanner::NodeToXY(void* node, int* x, int* y)
{
	Ped::Cell* cell = static_cast<Ped::Cell*>(node);
	if (cell == NULL)
		return;
	grid->getIndexByCell(cell, *x, *y);
	//*x = cell->idX();
	//*y = cell->idY();
}

void PathPlanner::AdjacentCost(void* node, micropather::MPVector< StateCost > *neighbors)
{
	int x, y;
	const int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
	const int dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	const float cost[8] = { 1.0f, 1.41f, 1.0f, 1.41f, 1.0f, 1.41f, 1.0f, 1.41f };

	NodeToXY(node, &x, &y);

	for (int i = 0; i < 8; ++i) {
		int nx = x + dx[i];
		int ny = y + dy[i];

		Ped::Cell* neighbor = grid->getCellByIndex(nx, ny);

		if (Passable(neighbor))
		{
			StateCost nodeCost = { neighbor, cost[i] };
			neighbors->push_back(nodeCost);
		}
	}
}

void PathPlanner::PrintStateInfo(void* node)
{
	int x, y;
	NodeToXY(node, &x, &y);
	printf("(%d,%d)", x, y);
}

int PathPlanner::SolvePath(double startX, double startY, double endX, double endY)
{
	Ped::Cell* startNode = grid->getCellByPosition(startX, startY);
	Ped::Cell* endNode = grid->getCellByPosition(endX, endY);

	return SolvePath(startNode, endNode);
}

int PathPlanner::SolvePath(Ped::Cell * start, Ped::Cell * end)
{
	clearPath();

	int result = 0;

	if (!start || !end)
		return MicroPather::NO_SOLUTION;

	//if (Passable(start))
	{
		float totalCost;

		result = pather->Solve(start, end, &path, &totalCost);

		if (path.size() > 1)
		{
			// Traversal the path and set every node's next cell
			for (unsigned int i = 0; i < path.size() - 1; i++)
			{
				Ped::Cell* node = static_cast<Ped::Cell*>(path[i]);
				Ped::Cell* nextNode = static_cast<Ped::Cell*>(path[i + 1]);

				// Stop when the cell has been planned on the specified destination
				if (!node->setNextCellInPath(end, nextNode))
					break;
			}
		}
	}
	return result;
}

void PathPlanner::Reset()
{
	pather->Reset();
}
