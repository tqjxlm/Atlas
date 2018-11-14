#ifndef _cell_h_
#define _cell_h_

#pragma warning(disable:4251)

#include <set>
#include <map>
#include <limits>
#include "agent.h"

using namespace std;

#define FLT_LMT 0.0001f
#undef max

namespace Ped {
	// Directions that allow path planning
	extern const Tvector pathDirections[8];

	// Directions that allow density flow
	extern const Tvector flowDirections[4];

	// Index of neighbours by direction
	extern const int cellNeighbourIndex[9][2];
}

namespace Ped {
	class Cell {
	public:
		struct CellAttribute {
			bool passable = true;
			float density;	// Currently, density is just the number of agents
		};

		enum DIRECTION {
			UP_LEFT,
			UP,
			UP_RIGHT,
			RIGHT,
			DOWN_RIGHT,
			DOWN,
			DOWN_LEFT,
			LEFT,
			SELF
		};

	public:
		// Decide which quarter the volocity lies
		// Index starts from top-left quarter
		static unsigned short getQuarter(const Tvector& velocity)
		{
			float x = static_cast<float>(velocity.x);
			float y = static_cast<float>(velocity.y);
			if (x < 0)
				if (y > 0)
					return abs(x) < abs(y) ? 0 : 7;
				else
					return abs(x) < abs(y) ? 5 : 6;
			else
				if (y > 0)
					return abs(x) < abs(y) ? 1 : 2;
				else
					return abs(x) < abs(y) ? 4 : 3;
		}

		// Directions that allow density flow
		static DIRECTION getOutDirection(const Tvector& velocity)
		{
			float x = static_cast<float>(velocity.x);
			float y = static_cast<float>(velocity.y);
			if (x < 0)
				if (y > 0)
					return abs(x) < abs(y) ? UP : LEFT;
				else
					return abs(x) < abs(y) ? DOWN : LEFT;
			else
				if (y > 0)
					return abs(x) < abs(y) ? UP : RIGHT;
				else
					return abs(x) < abs(y) ? DOWN : RIGHT;
		}

	public:
		Cell(float x, float y);
		Cell(float x, float y, int idX, int idY);

		void enter(const Agent* agent) { _agents.insert(agent); }
		void leave(const Agent* agent) { _agents.erase(agent); }

		const set<const Agent*>& getAgents() { return _agents; }
		CellAttribute& attribute() { return _attrib; }
		const CellAttribute& attribute() const { return _attrib; }

		const float x() const { return _x; }
		const float y() const { return _y; }
		const int idX() const { return _idX; }
		const int idY() const { return _idY; }
		const Ped::Tvector size() const { return Ped::Tvector(width, height); }
		const Ped::Tvector& center() const { return _center; }
		bool& changed() { return _dirtyState; }

		void reset() { _nextCellInPath.clear(); }

		static void setSize(float w, float h);

		static float getWidth() { return width; }
		static float getHeight() { return height; }

		void updateAttribute() { updateDensity(static_cast<float>(_agents.size())); }

		void updateDensity(float density) { _attrib.density = density; }

		// Return false if the path has been planned
		bool setNextCellInPath(const Cell* destination, Cell* next);

		Cell* getNextCellInPath(const Cell* destination)
		{
			if (_nextCellInPath.find(destination) == _nextCellInPath.end())
				return NULL;
			else
				return _nextCellInPath[destination];
		}

		// Compute the timestep before agent exits the cell
		// Velocity should be the in the form of one timestep
		float getExitTime(const Tvector& position, const Tvector& velocity)
		{
			switch (getOutDirection(velocity))
			{
			case(UP):
				return velocity.y < FLT_LMT ? std::numeric_limits<float>::max() : (_y - (float)position.y) / (float)velocity.y;
			case(DOWN):
				return velocity.y < FLT_LMT ? std::numeric_limits<float>::max() : (_y + height - (float)position.y) / (float)velocity.y;
			case(LEFT):
				return velocity.x < FLT_LMT ? std::numeric_limits<float>::max() : (_x - (float)position.x) / (float)velocity.x;
			case(RIGHT):
				return velocity.x < FLT_LMT ? std::numeric_limits<float>::max() : (_x + width - (float)position.x) / (float)velocity.x;
			default:
				return -1;
			}
		}

		DIRECTION getDirectionTo(const Cell* cell)
		{
			// Target coord
			int x = cell->idX();
			int y = cell->idY();

			switch (x - idX())
			{
			case(-1):
				return y - idY() == -1 ? UP_LEFT : (y - idY() == 0 ? LEFT : DOWN_LEFT);
			case(0):
				return y - idY() == -1 ? UP : (y - idY() == 0 ? SELF : DOWN);
			case(1):
				return y - idY() == -1 ? UP_RIGHT : (y - idY() == 0 ? RIGHT : DOWN_RIGHT);
			default:
				return SELF;
			}
		}

		float getAdjacentDistanceTo(const Tvector& pos)
		{
			if (pos.x < _x)
				return _x - static_cast<float>(pos.x);
			if (pos.x > _x + width)
				return static_cast<float>(pos.x) - (_x + width);
			if (pos.y < _y)
				return _y - static_cast<float>(pos.y);
			if (pos.y > _y + height)
				return static_cast<float>(pos.y) - (_y + height);

			return 0;
		}

		// Register a density change at the specified time, true for inflow, false for outflow
		void pushChangeInList(float timeStep, bool flowIn)
		{
			if (timeStep > 0 && timeStep < sizeof(_changeList) / sizeof(_changeList[0]))
				_changeList[static_cast<int>(timeStep)] += flowIn ? 1 : -1;
		}

		void resetChangeList()
		{
			std::fill(_changeList, _changeList + sizeof(_changeList) / sizeof(_changeList[0]), 0);
		}

		// Predict density after a certain time step
		float getDensityPrediction(float timeStep)
		{
			float density = _attrib.density;
			if (timeStep <= 0)
				return density;

			for (int i = 0; i < sizeof(_changeList) / sizeof(_changeList[0]) && i < timeStep; i++)
			{
				density += _changeList[i];
			}

			return density;
		}

	private:
		static float width, height, area;
		static float maxDensity;

		float _x, _y;
		int _idX, _idY;
		Ped::Tvector _center;

		set<const Agent*> _agents;
		bool _dirtyState;
		CellAttribute _attrib;

		int _changeList[10]{};

		// Path cache: next cell for each destination
		map<const Cell*, Cell*> _nextCellInPath;
	};
}

#endif
