#ifndef SCENE_H
#define SCENE_H

#pragma warning(disable:4251)

#include "libpedsim/ped_scene.h"

#include "config.h"
//#include "tree.h"
#include "agent.h"

using namespace std;

class PathPlanner;

namespace Ped {
	class Grid;
	class Cell;

	class Scene : public Tscene {
		friend class Agent;
		friend class Tagent;
		//friend class Tree;

	public:
		Scene(double x, double y, double w, double h);

		~Scene();

		virtual void update();
		virtual void addAgent(Tagent* a);
		virtual void addObstacle(Tobstacle* o);
		virtual void moveAgents(double h);

		Grid* getGrid() { return grid; }

		void setPathPlanner(PathPlanner* planner) { pathPlanner = planner; }
		PathPlanner* getPathPlanner() { return pathPlanner; }

	protected:
		void moveAgent(const Tagent *a);

	protected:
		Grid* grid;
		PathPlanner* pathPlanner;
		//void addAgent(Agent *a);

	//protected:
	//	void moveAgent(const Agent *a) {
	//		Ped::Tscene::moveAgent(a);
	//	}
	};
}

#endif