#include "obstacle.h"

using namespace Ped;

Obstacle::Obstacle(double pax, double pay, double pbx, double pby) : Tobstacle(pax, pay, pbx, pby) 
{
};

void Obstacle::setPosition(double pax, double pay, double pbx, double pby) 
{
	Tobstacle::setPosition(pax, pay, pbx, pby); 
};

