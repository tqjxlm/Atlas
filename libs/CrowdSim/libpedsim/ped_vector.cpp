//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#include "ped_vector.h"

#include <cmath>
#include <string>

#ifdef _WIN32
static const double M_PI = 3.14159265358979323846;
#endif

bool operator==(const Ped::Tvector& vector1In, const Ped::Tvector& vector2In) {
	return (vector1In.x == vector2In.x)
		&& (vector1In.y == vector2In.y)
		&& (vector1In.z == vector2In.z);
}


bool operator!=(const Ped::Tvector& vector1In, const Ped::Tvector& vector2In) {
	return (vector1In.x != vector2In.x)
		|| (vector1In.y != vector2In.y)
		|| (vector1In.z != vector2In.z);
}


Ped::Tvector operator+(const Ped::Tvector& vector1In, const Ped::Tvector& vector2In) {
	return Ped::Tvector(
		vector1In.x + vector2In.x,
		vector1In.y + vector2In.y,
		vector1In.z + vector2In.z);
}


Ped::Tvector operator-(const Ped::Tvector& vector1In, const Ped::Tvector& vector2In) {
	return Ped::Tvector(
		vector1In.x - vector2In.x,
		vector1In.y - vector2In.y,
		vector1In.z - vector2In.z);
}


Ped::Tvector operator-(const Ped::Tvector& vectorIn) {
	return Ped::Tvector(
		-vectorIn.x,
		-vectorIn.y,
		-vectorIn.z);
}


Ped::Tvector operator*(double factor, const Ped::Tvector& vector) {
	return vector.scaled(factor);
}

double Ped::Tvector::angleTo(const Tvector &other) const
{
	double angleThis = polarAngle();
	double angleOther = other.polarAngle();

	// compute angle
	double diffAngle = angleOther - angleThis;
	// → normalize angle
	if (diffAngle > M_PI) diffAngle -= 2 * M_PI;
	else if (diffAngle <= -M_PI) diffAngle += 2 * M_PI;

	return diffAngle;
}
