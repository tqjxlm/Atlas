#pragma once
#pragma warning(disable:4251)

#include <CrowdSim/CrowdSim.h>

#include <queue>
#include <QObject>

#include <osg/MatrixTransform>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

// OSG instance for a single agent
// TODO: Need to implement OSG instancing?
class AgentInstance {
public:
	osg::ref_ptr<osg::PositionAttitudeTransform> instance;
	const Ped::Agent* agent;
	osg::ref_ptr<osg::PositionAttitudeTransform> desiredIndicator;
};

// A source of agents that generates agents at interval
class AgentGenerator {
public:
	Ped::AgentGroup* group;
	osg::ref_ptr<osg::Group> intanceGroup;
	float x, y;
	float width, height;
	QTimer* generateTimer;
	float generateRate;
	int generateNumber;
};

class CrowdSimRenderer : public QObject, public osg::MatrixTransform
{
	Q_OBJECT

public:
	CrowdSimRenderer(osg::ref_ptr<osg::Node> model);
	~CrowdSimRenderer();

	void initTimer();

	void setBounding(osg::Vec2 upperLeft, osg::Vec2 range)
	{
		_upperLeft = upperLeft;
		_range = range;
	}

	osg::ref_ptr<osg::MatrixTransform> getRenderRoot() { return this; }
	std::vector<AgentGenerator*> getAgentSources() { return _sources; }
	Ped::Scene* getScene() { return _simulator->getScene(); }

	void setNumAgents(int totalCount) { _numAgents = totalCount; }

	void generateAgents(AgentGenerator* source);

signals:
	void setAgentCount(int);
	void pathNotFound();

public slots:
	void nextStep();
	void cleanUp();
	void initSimulation();
	void startSimulation();
	void pauseSimulation();
	void resetSimulation();
	void resetPather();

	void addWayPoint(float x, float y, float r);
	void addObstacle(float x1, float y1, float x2, float y2);
	void addAgentGroup(float generateRate, float x, float y, float width, float height);
	void updatePath(Ped::AgentGroup* agentGroup, float xStart, float yStart, float xEnd, float yEnd);

	void setDebugMode(bool enable);

	void setSimStep(double step) { _config.simh = step; }

	void getSimStep(double& step, double& min, double& max)
	{
		step = _config.simh;
		min = MIN_SIM_STEP;
		max = MAX_SIM_STEP;
	}

	void setSimDensity(double density) 
	{ 
		_config.densityUnpassable = density; 
		_config.densityRestricted = density / 4;
	}

	void getSimDensity(double& density, double& min, double& max)
	{
		density = _config.densityUnpassable;
		min = MIN_DENSITY_UNPASSABLE;
		max = MAX_DENSITY_UNPASSABLE;
	}

	void setPedDistance(double dist) { _config.simPedForce = dist; }

	void getPedDistance(double& dist, double& min, double& max)
	{
		dist = _config.simPedForce;
		min = MIN_PED_DISTANCE;
		max = MAX_PED_DISTANCE;
	}

	void setSimRate(float rate) { _targetRate = rate; }

	void getSimRate(float& rate, float& min, float& max)
	{
		rate = _targetRate;
		min = MIN_SIM_RATE;
		max = MAX_SIM_RATE;
	}

	void setSimRate(int rate) { setSimRate((float)rate); }
	void setPedDistance(int dist) { setPedDistance((double)dist); }
	void setSimDensity(int density) { setSimDensity((double)density); }

private:
	void initRenderRoot();
	osg::ref_ptr<osg::PositionAttitudeTransform> makeArrowIndicator(int type);

	// Visualize a vector in form of a rotation or a visable arrow
	// Index 0 is the agent model, Index > 0 are vector arrows
	void visualizeVector(AgentInstance& agent, osg::Vec3 vec, int mode, int index, float minSpeed);

private:
	const float MAX_SIM_RATE = 60;
	const float MIN_SIM_RATE = 1;
	const double MAX_PED_DISTANCE = 10;
	const double MIN_PED_DISTANCE = 0.1;
	const double MAX_SIM_STEP = 1.0;
	const double MIN_SIM_STEP = 0.01;
	const double MIN_DENSITY_UNPASSABLE = 1.0;
	const double MAX_DENSITY_UNPASSABLE = 8.0;

	bool _isSimulating = false;
	bool _debugMode = false;

	Ped::Config _config;
	CrowdSim* _simulator;

	int _numAgents;
	float _targetRate;

	QTimer* _cleanTimer;

	osg::Vec2 _upperLeft;
	osg::Vec2 _range;

	std::vector<AgentInstance> _agents;
	std::vector<AgentGenerator*> _sources;
	std::queue<AgentGenerator*> _generatingList;
	Ped::AgentGroup* _currentAgentGroup;

	osg::ref_ptr<osg::Node> _agentModel;
	osg::ref_ptr<osg::Node> _debugArrows[3];
};

