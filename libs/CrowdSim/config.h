#ifndef _config_h_
#define _config_h_
 
namespace Ped {
	class Config {

	public:
		float simWallForce = 1.0f;
		float simPedForce = 5.0f;
		float maxLookAheadTime = 2.0f;		// Max time range for agents to predict density changes
		float densityRestricted = 1.0f;	// Agents feels pressure if density exceeds this
		float densityUnpassable = 4.0f;	// Agents cannot move in if density exceeds this
		float willLimit = 3.0f;  // Agents will not change prefered direction if their will is lower than this
		double simh = 0.15;	// Simulation step size
		int generateNumber = 5;
		float inertiaLimit = 0.2f;		// Min acceleration that allows agents to change directions
		float maxAcceleration = 1.5f;		// Max acceleration that agents can adopt
	};

}
#endif
