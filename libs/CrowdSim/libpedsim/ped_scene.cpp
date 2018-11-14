//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#include "ped_scene.h"
#include "ped_agent.h"
#include "ped_obstacle.h"
#include "ped_tree.h"
#include "ped_waypoint.h"
#include "ped_outputwriter.h"

#include <cstddef>
#include <algorithm>
#include <stack>

using namespace std;


/// Default constructor. If this constructor is used, there will be no quadtree created.
/// This is faster for small scenarios or less than 1000 Tagents.
/// \date    2012-01-17
Ped::Tscene::Tscene() : tree(NULL), timestep(0) {};


/// Constructor used to create a quadtree statial representation of the Tagents. Use this
/// constructor when you have a sparsely populated world with many agents (>1000).
/// The agents must not be outside the boundaries given here. If in doubt, use an initial
/// boundary that is way to big.
/// \todo    Get rid of that limitation. A dynamic outer boundary algorithm would be nice.
/// \date    2012-01-17
/// \param left is the left side of the boundary
/// \param top is the upper side of the boundary
/// \param width is the total width of the boundary. Basically from left to right.
/// \param height is the total height of the boundary. Basically from top to down.
Ped::Tscene::Tscene(double left, double top, double width, double height) : Tscene() {
    tree = new Ped::Ttree(this, 0, left, top, width, height);
}

/// Destructor
/// \date    2012-02-04
Ped::Tscene::~Tscene() {
    delete(tree);
}

void Ped::Tscene::clear() {
    // clear tree
    treehash.clear();
    tree->clear();

    // remove all agents
    for(Ped::Tagent* currentAgent : agents)
        delete currentAgent;
    agents.clear();

    // remove all obstacles
    for(Ped::Tobstacle* currentObstacle : obstacles)
        delete currentObstacle;
    obstacles.clear();

    // remove all waypoints
    for(Ped::Twaypoint* currentWaypoint : waypoints)
        delete currentWaypoint;
    waypoints.clear();
}

/// Used to add a Tagent to the Tscene.
/// \date    2012-01-17
/// \warning addAgent() does call Tagent::setscene() to assign itself to the agent.
/// \param   *a A pointer to the Tagent to add.
void Ped::Tscene::addAgent(Ped::Tagent *a) {
    // add agent to scene
    // (take responsibility for object deletion)
    agents.push_back(a);
    a->setscene(this);
    if (tree != NULL)
        tree->addAgent(a);
}

/// Used to add a Tobstacle to the Tscene.
/// \date    2012-01-17
/// \param   *o A pointer to the Tobstacle to add.
/// \note    Obstacles added to the Scene are not deleted if the Scene is destroyed. The reason for this is because they could be member of another Scene theoretically.
void Ped::Tscene::addObstacle(Ped::Tobstacle *o) {
    // add obstacle to scene
    // (take responsibility for object deletion)
    obstacles.push_back(o);

    // then output their new position if an OutputWriter is given.
	for (auto ow : outputwriters) {
		ow->drawObstacle(*o);
	}
}

void Ped::Tscene::addWaypoint(Ped::Twaypoint* w) {
    // add waypoint to scene
    // (take responsibility for object deletion)
    waypoints.push_back(w);

    // then output their new position if an OutputWriter is given.
	for (auto ow : outputwriters) {
      ow->drawWaypoint(*w);
    }
}


/// Remove an agent from the scene.
/// \warning Used to delete the agent. I don't think Tscene has ownership of the assigned objects. Will not delete from now on.
bool Ped::Tscene::removeAgent(Ped::Tagent *a) {
    auto it = find(agents.begin(), agents.end(), a);
    if (it == agents.end())
        return false;

    // remove agent from the tree
    if (tree != NULL)
        tree->removeAgent(a);
    
    // remove agent from the scene, report succesful removal
    agents.erase(it);

    // notify the outputwriter
	for (auto ow : outputwriters) {
      Tagent aa = *a;
      ow->removeAgent(aa);
    }

    return true;
}

/// Remove an obstacle from the scene.
/// \warning Used to delete the obstacle. I don't think Tscene has ownership of the assigned objects. Will not delete from now on.
bool Ped::Tscene::removeObstacle(Ped::Tobstacle *o) {
    auto it = find(obstacles.begin(), obstacles.end(), o);
    if (it == obstacles.end())
      return false;

    // remove obstacle from the scene, report succesful removal
    obstacles.erase(it);
    return true;
}

/// Remove a waypoint from the scene.
/// \warning Used to delete the waypoint. I don't think Tscene has ownership of the assigned objects. Will not delete from now on.
bool Ped::Tscene::removeWaypoint(Ped::Twaypoint* w) {
  /* Not sure we want that! 
    // remove waypoint from all agents
    for(vector<Tagent*>::iterator iter = agents.begin(); iter != agents.end(); ++iter) {
        Tagent *a = (*iter);
        a->removeWaypoint(w);
    }
  */

    auto it = find(waypoints.begin(), waypoints.end(), w);
    if (it == waypoints.end())
        return false;

    // remove waypoint from the scene, report succesful removal
    waypoints.erase(it);
    return true;
}


/// This is a convenience method. It calls Ped::Tagent::move(double h) for all agents in the Tscene.
/// \date    2012-02-03
/// \param   h This tells the simulation how far the agents should proceed.
/// \see     Ped::Tagent::move(double h)
void Ped::Tscene::moveAgents(double h) {
    // update timestep
    timestep++;

    // first update forces
    for (Tagent* agent : agents) agent->computeForces();

    // then move agents according to their forces
    for (Tagent* agent : agents) agent->move(h);

    // then output their new position if an OutputWriter is given.
	//for (auto ow : outputwriters) {
 //     ow->writeTimeStep(timestep);
 //     for (Tagent* agent : agents) ow->drawAgent(*agent);
 //   }
}

/// Internally used to update the quadtree.
/// \date    2012-01-28
/// \param
void Ped::Tscene::placeAgent(const Ped::Tagent *a) {
    if (tree != NULL)
        tree->addAgent(a);
}

/// Moves a Tagent within the tree structure. The new position is taken from the agent. So it
/// basically updates the tree structure for that given agent. Ped::Tagent::move(double h) calls
/// this method automatically.
/// \date    2012-01-28
/// \param   *a the agent to move.
void Ped::Tscene::moveAgent(const Ped::Tagent *a) {
    if (tree != NULL)
        treehash[a]->moveAgent(a);
}

/// This triggers a cleanup of the tree structure. Unused leaf nodes are collected in order to
/// save memory. Ideally cleanup() is called every second, or about every 20 timestep.
/// \date    2012-01-28
void Ped::Tscene::cleanup() {
    if (tree != NULL)
        tree->cut();
}

/// Returns the list of neighbors within dist of the point x/y. This
/// can be the position of an agent, but it is not limited to this.
/// \date    2012-01-29
/// \return  The list of neighbors
/// \param   x the x coordinate
/// \param   y the y coordinate
/// \param   dist the distance around x/y that will be searched for agents (search field is a square in the current implementation)
set<const Ped::Tagent*> Ped::Tscene::getNeighbors(double x, double y, double dist) const {
    // if there is no tree, return all agents
    if (tree == NULL)
        return set<const Ped::Tagent*>(agents.begin(), agents.end());

    // create the output list
    list<const Ped::Tagent*> neighborList;
    getNeighbors(neighborList, x, y, dist);

    // copy the neighbors to a set
    return set<const Ped::Tagent*>(neighborList.begin(), neighborList.end());
}

void Ped::Tscene::getNeighbors(list<const Ped::Tagent*>& neighborList, double x, double y, double dist) const {
    stack<Ped::Ttree*> treestack;

    treestack.push(tree);
    while(!treestack.empty()) {
        Ped::Ttree *t = treestack.top();
        treestack.pop();
        if (t->isleaf) {
            t->getAgents(neighborList);
        }
        else {
            if (t->tree1->intersects(x, y, dist)) treestack.push(t->tree1);
            if (t->tree2->intersects(x, y, dist)) treestack.push(t->tree2);
            if (t->tree3->intersects(x, y, dist)) treestack.push(t->tree3);
            if (t->tree4->intersects(x, y, dist)) treestack.push(t->tree4);
        }
    }
}
