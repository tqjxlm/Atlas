#ifndef _agent_h_
#define _agent_h_

#pragma warning(disable:4251)

#include "libpedsim/ped_agent.h"
#include "libpedsim/ped_vector.h"

#include "config.h"

using namespace std;

namespace Ped {
	class Waypoint;
	class PathNode;
	class Agent;
	class Cell;

	// A group of agents that share waypoints and path info
	class AgentGroup {
	public:
		std::vector<Agent*> agents;
		std::vector<Waypoint*> wayPoints;
		std::vector<Cell*> pathNodes;		// Waypoints as cell

		// The basic path that is adopted if there is only one agent
		//std::vector<Cell*> referencePath;	
		std::vector<Cell*> currentPath;
	};
}

namespace Ped {
	class Config;
	class Cell;
	class Grid;
	//class Scene;

	class Agent : public Tagent {
	public:
		Agent();

		static void setConfig(Config* config) { _config = config; }
		static void setGrid(Grid* grid) { _grid = grid; }

		virtual void setGroup(const Ped::AgentGroup* group) { _group = group; }

		virtual void move(double h);
		virtual void computeForces();
		virtual void setInterval(int interval);
		int getTimeline() const { return _timeLine; }

		// Force that keeps the agent from each other (near range)
		virtual Tvector socialForce(const set<const Tagent*> &neighbors);

		// Force that keeps the agent away from obstacles
		virtual Tvector obstacleForce(const set<const Tagent*> &neighbors);

		const Tvector& getSocialForce() const { return socialforce; }
		const Tvector& getDesiredForce() const { return desiredforce; }
		const Tvector& getObstacleForce() const { return obstacleforce; }

		// Force that leads to the destination while maintaining a uniform density field
		virtual Tvector desiredForce();

		// Direction obtained from path planner
		virtual int plannedDirection();

		// Correct direction according to density
		virtual unsigned densityCorrection(int planned);

	private:
		static Config* _config;
		static Grid* _grid;
		std::vector<Cell*> _gridWayPoints;
		const AgentGroup* _group;
		int _pathIndex;

		// Asyc decision making
		int _timeLine;
		int _wakeInterval;
		bool _firstShot;

		// Time steps needed to leave the current cell
		float _exitTime;

		// Cell to enter after leaving the current one, must be direct adjacent
		Cell* _nextCell;	
		Cell* _currentCell;
	};
}

#endif
