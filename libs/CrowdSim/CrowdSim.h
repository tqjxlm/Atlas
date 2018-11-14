#pragma once

#pragma warning(disable:4251)

#include "scene.h"

#include "config.h"

namespace Ped {
	class Agent;
	class Waypoint;
	class Obstacle;
}

class PathPlanner;

class CrowdSim
{
public:
	CrowdSim(Ped::Config& config);
	~CrowdSim();

	// Add obstacle as line
	void addObstacle(double x1, double y1, double x2, double y2);

	// Add a group of agents scattered in a rectangle
	Ped::AgentGroup* addAgentGroup(int numAgents, double x, double y, double dx, double dy);

	std::vector<Ped::Agent*> addAgents(Ped::AgentGroup* agentGroup, int numAgents, double x, double y, double dx, double dy);

	// Asign way points to the last agent group
	void addWaypoint(Ped::AgentGroup* agentGroup, double x, double y, double r);

	void updateScene();

	void moveAllAgents() { _scene->moveAgents(_config.simh); }

	int findPath(Ped::AgentGroup* agentGroup, Ped::Waypoint* start, Ped::Waypoint* end);

	int findPath(Ped::AgentGroup* agentGroup, double xStart, double yStart, double xEnd, double yEnd);

	void cleanUp() { _scene->cleanup(); }

	Ped::Scene* getScene() { return _scene; }

	void init(double x, double y, double w, double h);

	void reset();

	void resetPather();

private:
	Ped::Config& _config;
	Ped::Scene* _scene;
	PathPlanner* _pathPlanner;
	std::vector<Ped::AgentGroup*> _agentGroups;
};

