//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#ifndef _ped_scene_h_
#define _ped_scene_h_ 1

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

#include <set>
#include <vector>
#include <map>
#include <list>

using namespace std;

namespace Ped {
  class OutputWriter;
  class Tagent;
  class Tobstacle;
  class Twaypoint;
}

EXPIMP_TEMPLATE template class LIBEXPORT std::vector<Ped::Tagent*>;
EXPIMP_TEMPLATE template class LIBEXPORT std::vector<Ped::Tobstacle*>;
EXPIMP_TEMPLATE template class LIBEXPORT std::vector<Ped::Twaypoint*>;
EXPIMP_TEMPLATE template class LIBEXPORT std::vector<Ped::OutputWriter*>;

namespace Ped {

    class Ttree;

    /// The Tscene class contains the spatial representation of the "world" the agents live in.
    /// Theoretically, in a continuous model, there are no boundaries to the size of the world.
    /// Agents know their position (the x/y co-ordinates). However, to find the nearest neighbors of
    /// an agent, it makes sense to put them in some kind of "boxes". In this implementation, the
    /// infinite world is divided by a dynamic quadtree structure. There are some CPU cycles
    /// required to update the structure with each agent position change. But the gain in looking
    /// up the neighbors is worth this. The quadtree structure only needs to be changed when an
    /// agent leaves its box, which might only happen every 100th or 1000th timestep, depending on
    /// the box size.
    /// The Tscene class needs an outer boundary in order to construct the initial box of the
    /// quadtree. Agents are not allowed to go outside that boundary. If you do not know how far
    /// they will walk, choose a rather big boundary box. The quadtree algorythm will dynamically
    /// assign smaller sub-boxes within if required.
    /// If all (most) agents walk out of a box, it is no longer needed. It can be colleted. If
    /// there are some agents left, they will be assigned to the box above in the hierarchy. You must
    /// trigger this collection process periodically by calling cleanup() manually.
    /// \author  chgloor
    /// \date    2010-02-12
    class LIBEXPORT Tscene {
        friend class Ped::Tagent;
        friend class Ped::Ttree;

    public:
        Tscene();
        Tscene(double left, double top, double width, double height);
        virtual ~Tscene();

        virtual void clear();

        virtual void addAgent(Tagent* a);
        virtual void addObstacle(Tobstacle* o);
        virtual void addWaypoint(Twaypoint* w);
        virtual bool removeAgent(Tagent* a);
        virtual bool removeObstacle(Tobstacle* o);
        virtual bool removeWaypoint(Twaypoint* w);

        virtual void cleanup();
        virtual void moveAgents(double h);

        set<const Ped::Tagent*> getNeighbors(double x, double y, double dist) const;
        const vector<Tagent*>& getAllAgents() const { return agents; };
        const vector<Tobstacle*>& getAllObstacles() const { return obstacles; };
        const vector<Twaypoint*>& getAllWaypoints() const { return waypoints; };

        void setOutputWriter(OutputWriter *ow) { outputwriters.push_back(ow); }

    protected:
        vector<Tagent*> agents;
        vector<Tobstacle*> obstacles;
        vector<Twaypoint*> waypoints;
        Ttree *tree;

        long int timestep;

        void placeAgent(const Ped::Tagent *a);
        void moveAgent(const Ped::Tagent *a);
        void getNeighbors(list<const Ped::Tagent*>& neighborList, double x, double y, double dist) const;
 
	private:
		vector<OutputWriter*> outputwriters;
		map<const Ped::Tagent*, Ttree*> treehash;

	};
}
#endif
