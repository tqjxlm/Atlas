//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#ifndef _ped_waypoint_h_
#define _ped_waypoint_h_ 1

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
#include <cstddef>

using namespace std;

namespace Ped {

    /// The waypoint classs
    /// \author  chgloor
    /// \date    2012-01-07
    class LIBEXPORT Twaypoint {
    public:
        enum WaypointType {
            TYPE_NORMAL = 0,
            TYPE_POINT = 1
        };

    public:
        Twaypoint();
        Twaypoint(double x, double y, double r);
        virtual ~Twaypoint();

        virtual Tvector getForce(double myx, double myy, double fromx, double fromy, bool *reached = NULL) const;
        virtual Tvector normalpoint(const Tvector& p, const Tvector& obstacleStart, const Tvector& obstacleEnd) const;
        virtual Tvector normalpoint(double p1, double p2, double oc11, double oc12, double oc21, double oc22) const;

        void setx(double px) { x = px; };
        void sety(double py) { y = py; };
        void setr(double pr) { r = pr; };
        void settype(WaypointType t) { type = t; };

        int getid() const { return id; };
        int gettype() const { return type; };
        double getx() const { return x; };
        double gety() const { return y; };
        double getr() const { return r; };

    protected:
        int id;                                           ///< waypoint number
        double x;                                         ///< position of the waypoint
        double y;                                         ///< position of the waypoint
        double r;                                         ///< position of the waypoint
        WaypointType type;                                ///< type of the waypoint
    };
}

#endif
