#include "CrowdSim.h"

// #include <windows.h>
#include <math.h>

// #include <ped_tree.h>

#include "agent.h"
#include "waypoint.h"
#include "obstacle.h"
#include "PathPlanner.h"
#include "grid.h"
#include "scene.h"

using namespace Ped;

CrowdSim::CrowdSim(Ped::Config &config):
  _scene(NULL), _pathPlanner(NULL), _config(config)
{
	Agent::setConfig(&_config);
}

CrowdSim::~CrowdSim()
{
	reset();

	if (_scene)
	{
		delete _scene;
	}

	if (_pathPlanner)
	{
		delete _pathPlanner;
	}
}

AgentGroup * CrowdSim::addAgentGroup(int numAgents, double x, double y, double dx, double dy)
{
  AgentGroup *currentAgentGroup = new AgentGroup;

	return currentAgentGroup;
}

inline void  getRandomPos(double &outX, double &outY, double x, double y, double dx, double dy)
{
	outX = x + rand() / (RAND_MAX / dx) - dx / 2;
	outY = y + rand() / (RAND_MAX / dy) - dy / 2;
}

std::vector<Agent *>  CrowdSim::addAgents(AgentGroup *agentGroup, int numAgents, double x, double y, double dx, double dy)
{
  std::vector<Agent *>  newAgents;

  auto  agents    = agentGroup->agents;
  auto  waypoints = agentGroup->wayPoints;

	for (int i = 0; i < numAgents; i++)
	{
		Agent *a = new Agent();

		// Scatter the agent in a (dx * dy) rectangle near (x, y)
    double  px, py;
    Cell   *initCell;
    int     count = 0;

		while (count < 20)
		{
			getRandomPos(px, py, x, y, dx, dy);
			initCell = _scene->getGrid()->getCellByPosition(px, py);

			if (initCell && initCell->attribute().passable)
      {
        break;
      }
    }

		// Give up if it takes to long to find a suitable cell
		if (!initCell)
		{
			continue;
		}

		a->setPosition(px, py, 0.0);

		newAgents.push_back(a);
		a->setGroup(agentGroup);

		_scene->addAgent(a);

		a->setscene(_scene);

		// Agents only change their minds every 10 steps
		a->setInterval(10);
	}

	agents.insert(agents.end(), newAgents.begin(), newAgents.end());

	return newAgents;
}

void  CrowdSim::addObstacle(double x1, double y1, double x2, double y2)
{
  Obstacle *obstacle = new Obstacle(x1, y1, x2, y2);

	_scene->addObstacle(obstacle);
}

void  CrowdSim::addWaypoint(AgentGroup *agentGroup, double x, double y, double r)
{
  Waypoint *wayPoint = new Waypoint(x, y, r);
  // for each (Agent* agent in agentGroup->agents)
  // {
  // agent->addWaypoint(wayPoint);
  // }
  auto &waypoints = agentGroup->wayPoints;

  // if (waypoints.size() > 0)
  // {
  // Waypoint* lastWayPoint = waypoints.back();
  // int PathResult = findPath(agentGroup, lastWayPoint, wayPoint);

  // auto& path = agentGroup->referencePath;

  // switch (PathResult)
  // {
  // case(MicroPather::SOLVED):
  // {
  // auto solvedPath = _pathPlanner->getSolvedPath();
  // for (unsigned i = 0; i < solvedPath.size(); i++)
  // path.push_back(static_cast<Cell*>(solvedPath[i]));
  // }
  // break;
  // case(MicroPather::NO_SOLUTION):
  // throw exception("No path found");
  // return;
  // default:
  ////path.push_back(std::vector<Ped::PathNode*>());
  // break;
  // }
  // }

	waypoints.push_back(wayPoint);
	agentGroup->pathNodes.push_back(_scene->getGrid()->getCellByPosition(x, y));
}

void  CrowdSim::updateScene()
{
	_scene->update();
}

int  CrowdSim::findPath(Ped::AgentGroup *agentGroup, Ped::Waypoint *start, Ped::Waypoint *end)
{
	return findPath(agentGroup, start->getx(), start->gety(), end->getx(), end->gety());
}

int  CrowdSim::findPath(Ped::AgentGroup *agentGroup, double xStart, double yStart, double xEnd, double yEnd)
{
  int  PathResult = _pathPlanner->SolvePath(
		xStart, yStart, xEnd, yEnd);
  auto &path = agentGroup->currentPath;

	switch (PathResult)
	{
  case (MicroPather::SOLVED):
	{
    const auto &solvedPath = _pathPlanner->getSolvedPath();
		path.clear();

		for (unsigned i = 0; i < solvedPath.size(); i++)
		{
      path.push_back(static_cast<Cell *>(solvedPath[i]));
		}
	}
  break;
  case (MicroPather::NO_SOLUTION):
		path.clear();
		break;
	default:
		path.clear();
		break;
	}

	return PathResult;
}

void  CrowdSim::init(double x, double y, double w, double h)
{
	if (_scene)
	{
		delete _scene;
	}

	if (_pathPlanner)
	{
		delete _pathPlanner;
	}

  _scene       = new Scene(x, y, w, h);
	_pathPlanner = new PathPlanner(_scene->getGrid());
	_scene->setPathPlanner(_pathPlanner);
}

void  CrowdSim::reset()
{
	if (_scene)
	{
		_scene->clear();
	}

	_pathPlanner->Reset();

  for (AgentGroup *group : _agentGroups)
	{
		delete group;
	}

	_agentGroups.clear();
}

void  CrowdSim::resetPather()
{
	_pathPlanner->Reset();
}
