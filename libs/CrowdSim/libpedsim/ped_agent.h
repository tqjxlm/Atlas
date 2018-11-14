//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//


#ifndef _ped_agent_h_
#define _ped_agent_h_ 1

//disable warnings on 255 char debug symbols
#pragma warning (disable : 4786)
//disable warnings on extern before template instantiation
#pragma warning (disable : 4231)

#ifdef _WIN32
#ifdef _DLL
#    define LIBEXPORT __declspec(dllexport)
#    define EXPIMP_TEMPLATE
#else
#    define LIBEXPORT __declspec(dllimport)
#    define EXPIMP_TEMPLATE extern
#endif
#else
#    define LIBEXPORT
#    define EXPIMP_TEMPLATE
#endif

#include "ped_vector.h"
#include "ped_waypoint.h"

#include <deque>
#include <set>
#include <vector>
#include <cstdio>

using namespace std;

EXPIMP_TEMPLATE template class LIBEXPORT std::deque<Ped::Twaypoint*>;

namespace Ped {
    class Tscene;

/// This is the main class of the library. It contains the Tagent, which eventually will move through the
/// Tscene and interact with Tobstacle and other Tagent. You can use it as it is, and access the agent's
/// coordinates using the getx() etc methods. Or, if you want to change the way the agent behaves, you can
/// derive a new class from it, and overwrite the methods you want to change. This is also a convenient way
/// to get access to internal variables not available though public methods, like the individual forces that
/// affect the agent.
/// \author  chgloor
/// \date    2003-12-26
    class LIBEXPORT Tagent {

    public:
        Tagent();
        virtual ~Tagent();

        virtual void computeForces();
        virtual void move(double stepSizeIn);
        virtual Tvector desiredForce();
        virtual Tvector socialForce(const set<const Ped::Tagent*> &neighbors);
        virtual Tvector obstacleForce(const set<const Ped::Tagent*> &neighbors);
        virtual Tvector lookaheadForce(Tvector desired, const set<const Ped::Tagent*> &neighbors);
        virtual Tvector myForce(Tvector desired, const set<const Ped::Tagent*> &neighbors);

        void setType(int t) { this->type = t; };
        int getType() const { return type; };

        void setVmax(double vmax);
        double getVmax();

        void setFollow(int id);
        int getFollow() const;

        int getid() const { return id; };

        void setPosition(double px, double py, double pz);
        void setPosition(const Tvector &pos) { p = pos; };
        Tvector getPosition() const { return p; }
        Tvector getVelocity() const { return v; }
        Tvector getAcceleration() const { return a; }

        void setfactorsocialforce(double f);
        void setfactorobstacleforce(double f);
        void setfactordesiredforce(double f);
        void setfactorlookaheadforce(double f);

        void setscene(Tscene* s);
        Tscene* getscene();

        void addWaypoint(Twaypoint* wp);
        bool removeWaypoint(const Twaypoint* wp);
        void clearWaypoints();
        deque<Twaypoint*> getWaypoints() { return waypoints; };

	//        void removeAgentFromNeighbors(const Tagent* agentIn);

        bool reachedDestination() const { return (destination == NULL); };
        void setWaypointBehavior(int mode) { waypointbehavior = mode; };

        enum WaypointBehavior {
            BEHAVIOR_CIRCULAR = 0,
            BEHAVIOR_ONCE = 1
        };


    protected:
        int id;                                           ///< agent number
        Tvector p;                                        ///< current position of the agent
        Tvector v;                                        ///< velocity of the agent
        Tvector a;                                        ///< current acceleration of the agent
        int type;
        double vmax;                                      ///< individual max velocity per agent
        int follow;

        Ped::Tvector desiredDirection;

        Ped::Tscene* scene;

        deque<Twaypoint*> waypoints;                      ///< coordinates of the next destinations
        Twaypoint* destination;                           ///< coordinates of the next destination
        Twaypoint* lastdestination;                       ///< coordinates of the last destination
        int waypointbehavior;                             ///< waypoints are round queues or not.

        bool mlLookAhead;

        double factordesiredforce;
        double factorsocialforce;
        double factorobstacleforce;
        double factorlookaheadforce;

        double obstacleForceSigma;

        Ped::Tvector desiredforce;
        Ped::Tvector socialforce;
        Ped::Tvector obstacleforce;
        Ped::Tvector lookaheadforce;
        Ped::Tvector myforce;

        double relaxationTime;

        double agentRadius;

	//        set<const Ped::Tagent*> neighbors;

        long timestep;
    };
}
#endif
