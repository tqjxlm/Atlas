#ifndef _obstacle_h_
#define _obstacle_h_

#pragma warning(disable:4251)

#include "libpedsim/ped_obstacle.h"

using namespace std;

/// \author  chgloor
/// \date    2012-01-17
namespace Ped {
	class Obstacle : public Tobstacle {
	private:

	public:
		Obstacle(double ax, double ay, double bx, double by);

		void setPosition(double ax, double ay, double bx, double by);
	};
}

#endif
