#ifndef _waypoint_h_
#define _waypoint_h_

#pragma warning(disable:4251)

#include <iostream>

#include "libpedsim/ped_waypoint.h"
#include "libpedsim/ped_vector.h"

using namespace std;

/// Class that descripts an waypoint object
/// \author  chgloor
/// \date    2012-01-07
namespace Ped {
	class Waypoint : public Ped::Twaypoint {
	private:

	public:
		Waypoint();
		Waypoint(double x, double y, double r);
		Ped::Tvector getForce(double myx, double myy, double fromx, double fromy, bool *reached); ///< returns the force into the direction of the waypoint
	};
}

#endif
