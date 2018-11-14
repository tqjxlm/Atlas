//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#ifndef _ped_obstacle_h_
#define _ped_obstacle_h_ 1

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

namespace Ped {

    /// Class that defines a Tobstacle object. An obstacle is, for now, always a wall with start and end coordinate.
    /// \author  chgloor
    /// \date    2012-01-17
    class LIBEXPORT Tobstacle {
    public:
        Tobstacle();
        Tobstacle(double ax, double ay, double bx, double by);
        Tobstacle(const Tvector& startIn, const Tvector& endIn);
        virtual ~Tobstacle();

        int getid() const { return id; };
        int gettype() const { return type; };
        double getax() const { return ax; };
        double getay() const { return ay; };
        double getbx() const { return bx; };
        double getby() const { return by; };
        Tvector getStartPoint() const;
        Tvector getEndPoint() const;

        virtual void setPosition(double ax, double ay, double bx, double by);
        virtual void setPosition(const Tvector& startIn, const Tvector& endIn);
        virtual void setStartPoint(const Tvector& startIn);
        virtual void setEndPoint(const Tvector& endIn);
        virtual void setType(int t) { type = t; };

        virtual Tvector closestPoint(double p1, double p2) const;
        virtual Tvector closestPoint(const Tvector& pointIn) const;
        virtual void rotate(double x, double y, double phi);

    protected:
        int id;									///< Obstacle number
        double ax;								///< Position of the obstacle
        double ay;								///< Position of the obstacle
        double bx;								///< Position of the obstacle
        double by;								///< Position of the obstacle
        int type;
    };
}

#endif
