#pragma warning(disable:4251)

#include <algorithm>
#include <random>
// #include <windows.h>

#include "agent.h"
#include "obstacle.h"
#include "config.h"
#include "scene.h"
#include "grid.h"
#include "PathPlanner.h"

using namespace std;
using namespace Ped;

#define MIN(a, b) a < b ? a : b
#define MAX(a, b) a > b ? a : b

Ped::Config *Agent::_config = NULL;
Ped::Grid *Ped::Agent::_grid = NULL;

Agent::Agent():
  Tagent(), _pathIndex(0), _firstShot(true)
{
	waypointbehavior = BEHAVIOR_ONCE;

	factordesiredforce = 1.5;

	agentRadius = 0.2f;
}

Ped::Tvector  Agent::socialForce(const set<const Ped::Tagent *> &neighbors)
{
  const float  posCoeff     = 0.3f;
  const float  inertiaCoeff = 0.05f;
  const float  speedCoeff   = 3.0f;
  const float  forceLimit   = 1.0f;
  Tvector      force;

  for (const Ped::Tagent *neighbor : neighbors)
	{
    const Agent *other = dynamic_cast<const Agent *>(neighbor);

    if (other->id == id) { continue; }

    Tvector  posDiff      = p - other->p;
    Tvector  posDirection = posDiff.normalized();
    float    distance     = static_cast<float>(posDiff.length());

		// Don't count in neighbours far away
    if (distance > 2.0f) { continue; }

    Tvector  relV     = v - other->v;
    float    speed    = static_cast<float>(v.length());
    float    relSpeed = static_cast<float>(relV.length());

		// Agents want to keep at distance with each other
		force += posDirection * posCoeff / pow(distance + 1.0f, 2);

		// Agents slow down quickly when running into others
    Tvector  speedInteraction = posDirection * Tvector::dotProduct(relV, -posDiff) * speedCoeff * log(relSpeed + 1);

		// Still agents are not easily pushed away
		if (speed > 0.5f)
    {
      force += speedInteraction;
    }
  }

	return force;
}

Ped::Tvector  Agent::obstacleForce(const set<const Ped::Tagent *> &neighbors)
{
  Tvector  force;

	// Search for nearby unpassable cells on the velocity direction
	for (int direction = Cell::UP; direction < Cell::SELF; direction += 2)
	{
    Cell *neighborCell = _grid->getCellByDirection(_currentCell, (Cell::DIRECTION)direction);

		if (!neighborCell->attribute().passable)
		{
      Tvector  dir = pathDirections[_currentCell->getDirectionTo(neighborCell)];
      double   dot = Tvector::dotProduct(dir, v);

			if (dot >= 0)
			{
				// When the agent is near an unpassable cell, push it away
        float  dist = neighborCell->getAdjacentDistanceTo(p);

				if (dist < Cell::getWidth() / 2)
        {
          force += -dir / (dist + 0.2);
        }
      }
		}
	}

	return force;
}

Ped::Tvector  Agent::desiredForce()
{
	// If final destination not reached
	if (_pathIndex < _group->pathNodes.size() - 1)
	{
		// A naive way to implement density flow that tries to maintain a uniform density field

		// 1. Get planned direction
    Cell::DIRECTION  planned = (Cell::DIRECTION)plannedDirection();

		if (planned != Cell::SELF)
		{
			// 2. Correct the direction according to density
			planned = (Cell::DIRECTION)densityCorrection(planned);

			// 3. Do move
      Cell *desiredCell = _grid->getCellByDirection(_currentCell, planned);

			if (desiredCell != _currentCell)
			{
				desiredDirection = pathDirections[_currentCell->getDirectionTo(desiredCell)];

				// Density constrained force: slow down when density > restricted density
        float  density    = _nextCell->getDensityPrediction(_exitTime);
        float  motivation = static_cast<float>(vmax);

				if (density > _config->densityRestricted)
        {
          motivation /= pow(density, 2);
        }

				return desiredDirection * motivation;
			}
		}
	}

	// Don't move
	return Tvector();
}

int  Ped::Agent::plannedDirection()
{
	// Get next planned direction
  int   nextPathIndex      = _pathIndex + 1;
  Cell *currentDestination = _group->pathNodes[nextPathIndex];

	// If reached current destination, change destination
	if (_currentCell == currentDestination)
	{
    _pathIndex   += 1;
		nextPathIndex = _pathIndex + 1;

		if (nextPathIndex == _group->pathNodes.size())
    {
      currentDestination = NULL;
    }
    else
    {
      currentDestination = _group->pathNodes[nextPathIndex];
    }
  }

	// Get the planned direction
	if (currentDestination)
	{
    Cell *nextCell = _currentCell->getNextCellInPath(currentDestination);

		if (!nextCell)
		{
			// Not planned yet, plan this cell
      Scene       *currentScene = (static_cast<Scene *>(scene));
      PathPlanner *pather       = currentScene->getPathPlanner();
      auto         pathResult   = pather->SolvePath(_currentCell, currentDestination);

			nextCell = _currentCell->getNextCellInPath(currentDestination);
		}

		if (!nextCell)
		{
			// If this happens, some thing must be wrong
			return Cell::SELF;
		}
		else
		{
			return _currentCell->getDirectionTo(nextCell);
		}
	}
	else
  {
    return Cell::SELF;
  }
}

unsigned  Ped::Agent::densityCorrection(int planned)
{
  const float  sparseIndex = 0.3f;

  Cell::DIRECTION  desired        = Cell::SELF;
  float            maxWill        = 0.0f;
  float            currentDensity = _currentCell->attribute().density;

	// Clockwise traversal on all directions
	for (int direction = 0; direction < Cell::SELF; direction++)
	{
    Cell *candidateCell = _grid->getCellByDirection(_currentCell, (Cell::DIRECTION)direction);

		// Predict a future density of the candidate cell
    float  targetDensity = candidateCell->getDensityPrediction(_exitTime);

		// Don't count oneself when evaluating density
    if ((candidateCell == _nextCell) && (_exitTime < _wakeInterval))
    {
			targetDensity -= 1;
    }

    float  densityDiff = currentDensity - targetDensity;

		// Reject denser or unpassable direction
    if ((densityDiff < 0) || !candidateCell->attribute().passable || (targetDensity >= _config->densityUnpassable))
    {
			continue;
    }

		// The effort needed to make a change
    int  directionDiff = abs(direction - planned);
    int  directionDist = directionDiff > Cell::SELF / 2 ? Cell::SELF - directionDiff : directionDiff;

		// Planned direction has higher will
		// By adjusting params {will, willLimit}, agents' behaviours may change drastically
    float  will;

		switch (directionDist)
		{
    case (0):
      will = 2.5f;
      break;
    case (1):
      will = 2.0f;
      break;
    case (2):
      will = 1.5f;
      break;
    case (3):
      will = 1.0f;
      break;
    case (4):
      will = 0.5f;
      break;
		}

		// Sparse direction has higher will
		will = will * (pow(densityDiff, sparseIndex) + 1);

		// Decide to move if will is strong enough
    if (((desired == Cell::SELF) || (will > maxWill)) && (will > _config->willLimit))
		{
			desired = static_cast<Cell::DIRECTION>(direction);
			maxWill = will;
		}
	}

	return desired;
}

void  Agent::move(double h)
{
	// A simple forward Euler integration
	// 1. Move the position according to the last velocity
	// 2. Calculate acceleration according to current forces
	// 3. Use acceleration to update velocity
  Tvector  p_desired = p + v * h;

	// As we use cells, the obstacle avoidance can be simple
  Cell *outCell = _grid->getCellByPosition(p_desired.x, p_desired.y);

	if (!outCell->attribute().passable)
	{
		p_desired = p;
    v         = Tvector();

		// Wake up immediately to make actions
		_timeLine = 1;
	}

	p = p_desired;

	// Register density change to cells
	_exitTime = _currentCell->getExitTime(p, v * h);
	_nextCell = _grid->getCellByDirection(_currentCell, _currentCell->getOutDirection(v));
	_currentCell->pushChangeInList(_exitTime, false);
	_nextCell->pushChangeInList(_exitTime, true);

	// Change direction once a while
	if (_timeLine <= 0)
	{
		a = desiredforce
        + socialforce
        + obstacleforce
        // + lookaheadforce * factorlookaheadforce
        // + myforce
    ;

		// Clamp acceleration to a reasonable value
    float  acceleration = static_cast<float>(a.length());

		if (acceleration < _config->inertiaLimit)
    {
      a = Ped::Tvector();
    }

    if (acceleration > _config->maxAcceleration)
    {
      a = a / acceleration * _config->maxAcceleration;
    }

		// FIXME: Why 0.5? Maybe there's a better way to implement inertia
		v = v * 0.5 + a * h; // prob rather (0.5 / h) * v

		// Clamp velocity to maximal speed
		if (v.length() > vmax)
    {
      v = v.normalized() * vmax;
    }

		_timeLine = _wakeInterval;

#ifdef _DEBUG
    // char buffer[80];
    // sprintf_s(buffer, "[Agent_%d]: a (%.1lf, %.1lf), v (%.1lf, %.1lf), p(%.1lf, %.1lf)\n",
    // id, a.x, a.y, v.x, v.y, p.x, p.y);
    // OutputDebugStringA(buffer);
#endif
	}

	// Notice scene of movement
  (dynamic_cast<Scene *>(scene))->moveAgent(this);

	_timeLine--;
}

void  Agent::computeForces()
{
  const double  neighborhoodRange = 1.4;

	_currentCell = _grid->getCellByAgent(this);

	if (_timeLine == 0)
	{
		setfactorsocialforce(_config->simPedForce);
		setfactorobstacleforce(_config->simWallForce);

		// Maybe put a limit on the number of neighbors?
    auto  neighbors = scene->getNeighbors(p.x, p.y, neighborhoodRange);

    desiredforce  = desiredForce() * factordesiredforce;
    socialforce   = socialForce(neighbors) * factordesiredforce;
		obstacleforce = obstacleForce(neighbors) * factorobstacleforce;
	}
}

void  Ped::Agent::setInterval(int interval)
{
	_wakeInterval = interval;
  _timeLine     = rand() % interval + 1;
}
