#include "scene.h"

//#include <ped_tree.h>
using namespace Ped;

#include "grid.h"

//void Scene::addAgent(Agent *a) {
//	// add agent to scene
//	// (take responsibility for object deletion)
//	agents.push_back(a);
//	a->setscene(this);
//	if (tree != NULL)
//		tree->addAgent(a);
//}

void Ped::Scene::update()
{
	grid->updateCellAttributes();
}

void Ped::Scene::moveAgents(double h) 
{
	// update timestep
	timestep++;

	// first update forces
	for (Tagent* agent : agents) 
		agent->computeForces();

	grid->resetCellChangeList();

	// then move agents according to their forces
	for (Tagent* agent : agents) 
		agent->move(h);
}

void Scene::addAgent(Ped::Tagent * a)
{
	Ped::Tscene::addAgent(a);
	grid->addAgent(static_cast<Agent*>(a));
}

void Scene::moveAgent(const Ped::Tagent * a)
{
	Ped::Tscene::moveAgent(a);
	grid->moveAgent(static_cast<const Agent*>(a));
}

Ped::Scene::Scene(double x, double y, double w, double h) : Tscene(x, y, w, h)
{
	//tree = new Tree(this, 0, x, y, w, h);
	grid = new Grid(x, y, w, h);
}

Ped::Scene::~Scene()
{
	delete grid;
}

void Ped::Scene::addObstacle(Tobstacle* o)
{
	Ped::Tscene::addObstacle(o);
	grid->addObstacle(o);
}
