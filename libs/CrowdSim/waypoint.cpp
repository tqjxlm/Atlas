//
// pedsim - A microscopic pedestrian simulation system. 
// Copyright (c) 2003 - 2012 by Christian Gloor
//                              

#include "waypoint.h"
#include "config.h"

#include <iostream>

using namespace std;
using namespace Ped;


/// Description: set intial values
/// \date    2012-01-07
Waypoint::Waypoint(double px, double py, double pr) : Twaypoint(px, py, pr) {
};


/// \date    2012-01-07
Waypoint::Waypoint() : Twaypoint() {
};


/// returns the force into the direction of the waypoint
/// \date    2012-01-10
Ped::Tvector Waypoint::getForce(double myx, double myy, double fromx, double fromy, bool *reached) {

	return Twaypoint::getForce(myx, myy, fromx, fromy, reached);
}
