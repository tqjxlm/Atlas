//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#include "ped_agent.h"
#include "ped_waypoint.h"
#include "ped_scene.h"
#include "ped_obstacle.h"

#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

using namespace std;


default_random_engine generator;


/// Default Constructor
/// \date    2003-12-29
Ped::Tagent::Tagent() {
    static int staticid = 0;
    id = staticid++;
    p.x = 0;
    p.y = 0;
    p.z = 0;
    v.x = 0;
    v.y = 0;
    v.z = 0;
    type = 0;

    destination = NULL;
    lastdestination = NULL;
    follow = -1;
    mlLookAhead = false;
    scene = NULL;

    // assign random maximal speed in m/s
    // normal distribution (mean 1.2, std 0.2)
    normal_distribution<double> distribution(1.2, 0.2);
    vmax = distribution(generator);

    factorsocialforce = 2.1;
    factorobstacleforce = 1.0; // parameter based on plausible pedsim output, not real measurement!
    factordesiredforce = 1.0;
    factorlookaheadforce = 1.0;

    obstacleForceSigma = 0.8;

    agentRadius = 0.2;

    relaxationTime = 0.5;

    waypointbehavior = BEHAVIOR_CIRCULAR;
}


/// Destructor
/// \date    2012-02-04
Ped::Tagent::~Tagent() {
}


/// Assigns a Tscene to the agent. Tagent uses this to iterate over all
/// obstacles and other agents in a scene.  The scene will invoke this function
/// when Tscene::addAgent() is called.
/// \date    2012-01-17
/// \warning Bad things will happen if the agent is not assigned to a scene. But usually, Tscene takes care of that.
/// \param   *s A valid Tscene initialized earlier.
void Ped::Tagent::setscene(Ped::Tscene *s) {
    scene = s;
}

/// Returns the Tscene assigned to the agent.
/// \date    2012-01-17
/// \warning Bad things will happen if the agent is not assigned to a scene. But usually, Tscene takes care of that.
/// \return   *s A Tscene initialized earlier, if one is assigned to the agent.
Ped::Tscene* Ped::Tagent::getscene() {
  return scene;
};


/// Adds a TWaypoint to an agent's list of waypoints. Twaypoints are stored in a
/// cyclic queue, the one just visited is pushed to the back again. There will be a
/// flag to change this behavior soon.
/// Adding a waypoint will also selecting the first waypoint in the internal list
/// as the active one, i.e. the first waypoint added will be the first point to
/// headt to, no matter what is added later.
/// \author  chgloor
/// \date    2012-01-19
void Ped::Tagent::addWaypoint(Twaypoint *wp) {
    waypoints.push_back(wp);
    destination = waypoints.front();
}


bool Ped::Tagent::removeWaypoint(const Twaypoint *wp) {
    // unset references
    if (destination == wp)
        destination = NULL;
    if (lastdestination == wp)
        lastdestination = NULL;

    // remove waypoint from list of destinations
    bool removed = false;
    for (int i = waypoints.size(); i > 0; --i) {
        Twaypoint* currentWaypoint = waypoints.front();
        waypoints.pop_front();
        if (currentWaypoint != wp) {
            waypoints.push_back(currentWaypoint);
            removed = true;
        }
    }

    return removed;
}


void Ped::Tagent::clearWaypoints() {
    // unset references
    destination = NULL;
    lastdestination = NULL;

    // remove all references to the waypoints
    // note: don't delete waypoints, because the scene is responsible
    for (int i = waypoints.size(); i > 0; --i) {
        waypoints.pop_front();
    }
}

/*
void Ped::Tagent::removeAgentFromNeighbors(const Ped::Tagent* agentIn) {
    // search agent in neighbors, and remove him
    set<const Ped::Tagent*>::iterator foundNeighbor = neighbors.find(agentIn);
    if (foundNeighbor != neighbors.end()) {
        neighbors.erase(foundNeighbor);
    }
}
*/ // no longer required

/// Sets the agent ID this agent has to follow. If set, the agent will ignore
/// its assigned waypoints and just follow the other agent.
/// \date    2012-01-08
/// \param   id is the agent to follow (must exist, obviously)
/// \todo    Add a method that takes a Tagent* as argument
void Ped::Tagent::setFollow(int id) {
    follow = id;
}


/// Gets the ID of the agent this agent is following.
/// \date    2012-01-18
/// \return  int, the agent id of the agent
/// \todo    Add a method that returns a Tagent*
int Ped::Tagent::getFollow() const {
    return follow;
}


/// Sets the maximum velocity of an agent (vmax). Even if pushed by other
/// agents, it will not move faster than this.
/// \date    2012-01-08
/// \param pvmax The maximum velocity. In scene units per timestep, multiplied by the simulation's precision h.
void Ped::Tagent::setVmax(double pvmax) {
    vmax = pvmax;
}

/// Gets the maximum velocity of an agent (vmax). Even if pushed by other
/// agents, it will not move faster than this.
/// \date    2016-08-10
/// \return The maximum velocity. In scene units per timestep, multiplied by the simulation's precision h.
double Ped::Tagent::getVmax() {
  return vmax;
}


/// Sets the agent's position. This, and other getters returning coordinates,
/// will eventually changed to returning a Tvector.
/// \date    2004-02-10
/// \param   px Position x
/// \param   py Position y
/// \param   pz Position z
void Ped::Tagent::setPosition(double px, double py, double pz) {
    p.x = px;
    p.y = py;
    p.z = pz;
}


/// Sets the factor by which the social force is multiplied. Values between 0
/// and about 10 do make sense.
/// \date    2012-01-20
/// \param   f The factor
void Ped::Tagent::setfactorsocialforce(double f) {
    factorsocialforce = f;
}


/// Sets the factor by which the obstacle force is multiplied. Values between 0
/// and about 10 do make sense.
/// \date    2012-01-20
/// \param   f The factor
void Ped::Tagent::setfactorobstacleforce(double f) {
    factorobstacleforce = f;
}


/// Sets the factor by which the desired force is multiplied. Values between 0
/// and about 10 do make sense.
/// \date    2012-01-20
/// \param   f The factor
void Ped::Tagent::setfactordesiredforce(double f) {
    factordesiredforce = f;
}


/// Sets the factor by which the look ahead force is multiplied. Values between
/// 0 and about 10 do make sense.
/// \date    2012-01-20
/// \param   f The factor
void Ped::Tagent::setfactorlookaheadforce(double f) {
    factorlookaheadforce = f;
}


/// Calculates the force between this agent and the next assigned waypoint.  If
/// the waypoint has been reached, the next waypoint in the list will be
/// selected.  At the moment, a visited waypoint is pushed back to the end of
/// the list, which means that the agents will visit all the waypoints over and
/// over again.  This behavior can be controlled by a flag using setWaypointBehavior().
/// \date    2012-01-17
/// \return  Tvector: the calculated force
Ped::Tvector Ped::Tagent::desiredForce() {

    // following behavior
    if (follow >= 0) {
        Tagent* followedAgent = scene->agents.at(follow);
        Twaypoint newDestination(followedAgent->getPosition().x, followedAgent->getPosition().y, 0);
        newDestination.settype(Ped::Twaypoint::TYPE_POINT);
        Ped::Tvector ef = newDestination.getForce(p.x, p.y, 0, 0);
        desiredDirection = Ped::Tvector(followedAgent->getPosition().x, followedAgent->getPosition().y);

        // walk with full speed if nothing else affects me
        return vmax * ef;
    }

    // waypoint management (fetch new destination if available)
    if ((destination == NULL) && (!waypoints.empty())) {
        destination = waypoints.front();
        waypoints.pop_front();

        if (waypointbehavior == Ped::Tagent::BEHAVIOR_CIRCULAR) {
            waypoints.push_back(destination);
        }
    }

    // Agent has reached last waypoint, or there never was one.
    //    if ((destination != NULL) && (waypoints.empty())) {
    //        destination = NULL;
    //    }

    // if there is no destination, don't move
    if (destination == NULL) {
        //        desiredDirection = Ped::Tvector();
        //        Tvector antiMove = -v / relaxationTime;
        //        return antiMove;
        // not shure about these lines above
        desiredDirection = Ped::Tvector(0, 0, 0);
    }

    bool reached = false;
    if  (destination != NULL) {
        if (lastdestination == NULL) {
            // create a temporary destination of type point, since no normal from last dest is available
            Twaypoint tempDestination(destination->getx(), destination->gety(), destination->getr());
            tempDestination.settype(Ped::Twaypoint::TYPE_POINT);
            desiredDirection = tempDestination.getForce(p.x,
                                                        p.y,
                                                        0,
                                                        0,
                                                        &reached);
        } else {
            desiredDirection = destination->getForce(p.x,
                                                     p.y,
                                                     lastdestination->getx(),
                                                     lastdestination->gety(),
                                                     &reached);
        }
    }

    // mark destination as reached for next time step
    if ((destination != NULL) && (reached == true)) {
        lastdestination = destination;
        destination = NULL;
    }

    // Compute force. This translates to "I want to move into that
    // direction at the maximum speed"
    Tvector force = desiredDirection.normalized() * vmax;
    //    cout << force.to_string() << endl;

    return force;
}


/// Calculates the social force between this agent and all the other agents
/// belonging to the same scene.  It iterates over all agents inside the scene,
/// has therefore complexity \f$O(N^2)\f$. A better agent storing structure in
/// Tscene would fix this. But for small (less than 10000 agents) scenarios,
/// this is just fine.
/// \date    2012-01-17
/// \return  Tvector: the calculated force
Ped::Tvector Ped::Tagent::socialForce(const set<const Ped::Tagent*> &neighbors) {
    // define relative importance of position vs velocity vector
    // (set according to Moussaid-Helbing 2009)
    const double lambdaImportance = 2.0;

    // define speed interaction
    // (set according to Moussaid-Helbing 2009)
    const double gamma = 0.35;

    // define speed interaction
    // (set according to Moussaid-Helbing 2009)
    const double n = 2;

    // define angular interaction
    // (set according to Moussaid-Helbing 2009)
    const double n_prime = 3;

    Tvector force;
    for (const Ped::Tagent* other: neighbors) {
        // don't compute social force to yourself
        if (other->id == id) continue;

        // compute difference between both agents' positions
        Tvector diff = other->p - p;

        // skip futher computation if they are too far away from each
        // other. Should speed up things.
        if (diff.lengthSquared() > 64.0) continue; // val to high --chgloor 20160630

        Tvector diffDirection = diff.normalized();

        // compute difference between both agents' velocity vectors
        // Note: the agent-other-order changed here
        Tvector velDiff = v - other->v;

        // compute interaction direction t_ij
        Tvector interactionVector = lambdaImportance * velDiff + diffDirection;
        double interactionLength = interactionVector.length();
        Tvector interactionDirection = interactionVector / interactionLength;

        // compute angle theta (between interaction and position difference vector)
        double theta = interactionDirection.angleTo(diffDirection);
        int thetaSign = (theta == 0) ? (0) : (theta / abs(theta));

        // compute model parameter B = gamma * ||D||
        double B = gamma * interactionLength;

        // According to paper, this should be the sum of the two forces...
//          force += -exp(-diff.length()/B)
//              * (exp(-pow(n_prime*B*theta,2)) * interactionDirection
//                  + exp(-pow(n*B*theta,2)) * interactionDirection.leftNormalVector());

        double forceVelocityAmount = -exp(-diff.length()/B - (n_prime*B*theta)*(n_prime*B*theta));
        double forceAngleAmount = -thetaSign * exp(-diff.length()/B - (n*B*theta)*(n*B*theta));

        Tvector forceVelocity = forceVelocityAmount * interactionDirection;
        Tvector forceAngle = forceAngleAmount * interactionDirection.leftNormalVector();

        force += forceVelocity + forceAngle;
    }

// Old code: (didn't follow papers)
//      const double maxDistance = 10.0;
//      const double maxDistSquared = maxDistance*maxDistance;
//
//      Ped::Tvector force;
//      for(set<const Ped::Tagent*>::iterator iter = neighbors.begin(); iter!=neighbors.end(); ++iter) {
//          const Ped::Tagent* other = *iter;
//
//          // don't compute social force to yourself
//          if (other->id == id)
//              continue;
//
//           // quick distance check
//          Ped::Tvector diff = other->p - p;
//          if ((abs(diff.x) < maxDistance)
//              && (abs(diff.y) < maxDistance)) {
//              double dist2 = diff.lengthSquared();
//
//              // ignore too small forces
//              if (dist2 < maxDistSquared) {
//                  double expdist = exp(-sqrt(dist2)/socialForceSigma);
//                  force += -expdist * diff;
//              }
//          }
//      }

    return force;
}


/// Calculates the force between this agent and the nearest obstacle in this scene.
/// Iterates over all obstacles == O(N).
/// \date    2012-01-17
/// \return  Tvector: the calculated force
Ped::Tvector Ped::Tagent::obstacleForce(const set<const Ped::Tagent*> &neighbors) {
    // obstacle which is closest only
    Ped::Tvector minDiff;
    double minDistanceSquared = INFINITY;

    for (const Tobstacle* obstacle : scene->obstacles) {
        Ped::Tvector closestPoint = obstacle->closestPoint(p);
        Ped::Tvector diff = p - closestPoint;
        double distanceSquared = diff.lengthSquared();  // use squared distance to avoid computing square root
        if (distanceSquared < minDistanceSquared) {
            minDistanceSquared = distanceSquared;
            minDiff = diff;
        }
    }

    double distance = sqrt(minDistanceSquared) - agentRadius;
    double forceAmount = exp(-distance/obstacleForceSigma);
    return forceAmount * minDiff.normalized();
}


/// Calculates the mental layer force of the strategy "look ahead". It is
/// implemented here in the physical layer because of performance reasons. It
/// iterates over all Tagents in the Tscene, complexity \f$O(N^2)\f$.
/// \date    2012-01-17
/// \return  Tvector: the calculated force
/// \param e is a vector defining the direction in which the agent should look
///          ahead to. Usually, this is the direction he wants to walk to.
Ped::Tvector Ped::Tagent::lookaheadForce(Ped::Tvector e, const set<const Ped::Tagent*> &neighbors) {
    const double pi = 3.14159265;
    int lookforwardcount = 0;
    for (set<const Ped::Tagent*>::iterator iter = neighbors.begin(); iter!=neighbors.end(); ++iter) {
        const Ped::Tagent* other = *iter;

        // don't compute this force for the agent himself
        if (other->id == id) continue;

        double distancex = other->p.x - p.x;
        double distancey = other->p.y - p.y;
        double dist2 = (distancex * distancex + distancey * distancey); // 2D
        if (dist2 < 400) { // look ahead feature
            double at2v = atan2(-e.x, -e.y); // was vx, vy  --chgloor 2012-01-15
            double at2d = atan2(-distancex, -distancey);
            double at2v2 = atan2(-other->v.x, -other->v.y);
            double s = at2d - at2v;
            if (s > pi) s -= 2*pi;
            if (s < -pi) s += 2*pi;
            double vv = at2v - at2v2;
            if (vv > pi) vv -= 2*pi;
            if (vv < -pi) vv += 2*pi;
            if (abs(vv) > 2.5) { // opposite direction
                if ((s < 0) && (s > -0.3)) // position vor mir, in meine richtung
                    lookforwardcount--;
                if ((s > 0) && (s < 0.3))
                    lookforwardcount++;
            }
        }
    }

    Ped::Tvector lf;
    if (lookforwardcount < 0) {
        lf.x = 0.5f *  e.y; // was vx, vy  --chgloor 2012-01-15
        lf.y = 0.5f * -e.x;
    }
    if (lookforwardcount > 0) {
        lf.x = 0.5f * -e.y;
        lf.y = 0.5f *  e.x;
    }
    return lf;
}


/// myForce() is a method that returns an "empty" force (all components set to 0).
/// This method can be overridden in order to define own forces.
/// It is called in move() in addition to the other default forces.
/// \date    2012-02-12
/// \return  Tvector: the calculated force
/// \param   e is a vector defining the direction in which the agent wants to walk to.
Ped::Tvector Ped::Tagent::myForce(Ped::Tvector e, const set<const Ped::Tagent*> &neighbors) {
    Ped::Tvector lf;
    return lf;
}

/// This is the first step of the 2-step update process used
/// here. First, all forces are computed, using the t-1 agent
/// positions as input. Once the forces are computed, all agent
/// positions for timestep t are updated at the same time.
void Ped::Tagent::computeForces() {
    const double neighborhoodRange = 20.0;
    auto neighbors = scene->getNeighbors(p.x, p.y, neighborhoodRange);

    desiredforce = desiredForce();
    if (factorlookaheadforce > 0) lookaheadforce = lookaheadForce(desiredDirection, neighbors);
    if (factorsocialforce > 0) socialforce = socialForce(neighbors);
    if (factorobstacleforce > 0) obstacleforce = obstacleForce(neighbors);
    myforce = myForce(desiredDirection, neighbors);
}


/// Does the agent dynamics stuff. In the current implementation a
/// simple Euler integration is used. As the first step, the new
/// position is calculated using t-1 velocity. Then, the new
/// contributing individual forces are calculated. This will then be
/// added to the existing velocity, which again is used during the
/// next time step. See e.g. https://en.wikipedia.org/wiki/Euler_method
///
/// \date 2003-12-29 
/// \param h Integration time step delta t
void Ped::Tagent::move(double h) {
  // internal position update = actual move
  //    p = p + v * h;
  Tvector p_desired = p + v * h;


  Ped::Tvector intersection;
  bool has_intersection = false;
  for (auto obstacle : scene->getAllObstacles()) {
    Ped::Tvector intersection;
    // Ped::Tvector surface = obstacle->getEndPoint() - obstacle->getStartPoint();
    // Ped::Tvector vd = surface.leftNormalVector().normalized() * 0.30; // min radius of agent

    // // walls left and right
    // if (Ped::Tvector::lineIntersection(p, p_desired, obstacle->getStartPoint()-vd, obstacle->getEndPoint()-vd, &intersection) == 1) {
    //   p_desired = intersection - (v*h).normalized()*0.1;
    // }
    // if (Ped::Tvector::lineIntersection(p, p_desired, obstacle->getStartPoint()+vd, obstacle->getEndPoint()+vd, &intersection) == 1) {
    //   p_desired = intersection - (v*h).normalized()*0.1;
    // }

    // // caps
    // if (Ped::Tvector::lineIntersection(p, p_desired, obstacle->getStartPoint()-vd, obstacle->getStartPoint()+vd, &intersection) == 1) {
    //   p_desired = intersection - (v*h).normalized()*0.1;
    // }
    // if (Ped::Tvector::lineIntersection(p, p_desired, obstacle->getEndPoint()-vd, obstacle->getEndPoint()+vd, &intersection) == 1) {
    //   p_desired = intersection - (v*h).normalized()*0.1;
    // }

    if (Ped::Tvector::lineIntersection(p, p_desired, obstacle->getStartPoint(), obstacle->getEndPoint(), &intersection) == 1) {
      p_desired = intersection - (v*h).normalized()*0.1;
    }
  }

  p = p_desired;  // update my position

  
  // weighted sum of all forces --> acceleration
  a = factordesiredforce * desiredforce
    + factorsocialforce * socialforce
    + factorobstacleforce * obstacleforce
    + factorlookaheadforce * lookaheadforce
    + myforce;

  // calculate the new velocity
  v = 0.5 * v + a * h; // prob rather (0.5 / h) * v

  // don't exceed maximal speed
  if (v.length() > vmax) v = v.normalized() * vmax;

  // notice scene of movement
  scene->moveAgent(this);
}
