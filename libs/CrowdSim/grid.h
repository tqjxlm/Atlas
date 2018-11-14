#ifndef _grid_h_
#define _grid_h_

#pragma warning(disable:4251)

#include <vector>
#include <map>
using namespace std;

//#include <Eigen/Dense>
//using namespace Eigen;

#include "cell.h"

static const double cellWidth = 2;
static const double cellHeight = 2;

namespace Ped {

	class Agent;
	class Tobstacle;

	class Grid {
	public:
		Grid(double x, double y, double w, double h);
		~Grid();

		virtual void updateCellAttributes();
		virtual void resetCellChangeList();
		virtual void addAgent(const Agent* agent);
		virtual void moveAgent(const Agent* agent);
		virtual void addObstacle(const Tobstacle* obstacle);

		Cell* getCellByPosition(double x, double y) const
		{
			if (x < minx)
				return NULL;
			if (y < miny)
				return NULL;

			unsigned int cellx = (unsigned int)((x - minx) / cellWidth);
			unsigned int celly = (unsigned int)((y - miny) / cellHeight);

			if (cellx >= cells.size())
				return NULL;
			if (celly >= cells[0].size())
				return NULL;

			return cells[cellx][celly];
		}

		Cell* getCellByIndex(int x, int y) const
		{
			if (x < 0 || x >= cells.size())
				return NULL;
			if (y < 0 || y >= cells[0].size())
				return NULL;

			return cells[x][y];
		}

		Cell* getCellByAgent(const Agent* agent) { return gridHash[agent]; }

		Cell* getCellByDirection(const Cell* cell, Cell::DIRECTION direction)
		{
			return getCellByIndex(
				cell->idX() + cellNeighbourIndex[direction][0],
				cell->idY() + cellNeighbourIndex[direction][1]);
		}

		vector<Cell*> getCellsInDirection(Cell* cell, Cell::DIRECTION direction, int count)
		{
			vector<Cell*> nearestNeighbours;
			for (int i = 0; i < count; i++)
			{
				nearestNeighbours.push_back(
					getCellByDirection(
						cell, (Cell::DIRECTION)((direction - count / 2 + i + Cell::SELF) % Cell::SELF)));
			}
			return nearestNeighbours;
		}

		void getIndexByCell(const Cell* cell, int& cellx, int& celly) const
		{
			//double x = cell->center().x;
			//double y = cell->center().y;

			//cellx = (int)((x - minx) / cellWidth);
			//celly = (int)((y - miny) / cellHeight);

			cellx = cell->idX();
			celly = cell->idY();
		}

		const Cell::CellAttribute& getCellAttribute(double x, double y);

		const vector< vector<Cell*> >& getCells() const { return cells; }

		void updateDensityFeild();

		//void solveUIC();

		//void nextIteration();

		//void solveQP();

	private:

		double minx;
		double miny;

		vector< vector<Cell*> > cells;
		map<const Agent*, Cell*> gridHash;

	private:
	/* 
		Variables for solving the flow field.

		LCP: z = Ax + b, where z = nextSigma, x = press
		QP: x'(Ax + b) with Ax + b >= 0
		QP by definition: 0.5x'Px + q'x with l <= Mx <= u
		
		Algorithm:
		0. Update all cell attributes
		1. Compute from cells to get Ax and b
		2. Solve Ax = A * x to get A
		3. Solve QP to get to get new x and new z
		4. Update x and z for all cells
		5. Iterate from 0 until new z is really close to z

	*/
		//Eigen::VectorXd* nextSigma = NULL;
		//Eigen::VectorXd* thisSigma = NULL;
		//Eigen::VectorXd* b = NULL;
		//Eigen::VectorXd* Ax = NULL;
		//Eigen::MatrixXd* A = NULL;
		//Eigen::VectorXd* press = NULL;
	};
}

#endif
