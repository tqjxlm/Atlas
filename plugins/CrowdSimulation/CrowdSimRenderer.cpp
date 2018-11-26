#include "CrowdSimRenderer.h"

#include <QMetaType>
#include <QTimer>

#include <osg/Shape>
#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>
#include <osg/Switch>
#include <osg/Geode>
#include <osg/Notify>

#include <CrowdSim/libpedsim/ped_vector.h>
#include <CrowdSim/agent.h>

Q_DECLARE_METATYPE(osg::ref_ptr<osg::MatrixTransform> );

inline static osg::Vec3  fromTvector(const Ped::Tvector &vector)
{
	return osg::Vec3(vector.x, vector.y, vector.z);
}

inline osg::ref_ptr<osg::PositionAttitudeTransform>  wrapPositionAttitude(osg::ref_ptr<osg::Node> child)
{
  osg::ref_ptr<osg::PositionAttitudeTransform>  node = new osg::PositionAttitudeTransform;
	node->addChild(child);

	return node;
}

class RendererStepCallBack: public osg::NodeCallback
{
public:
  RendererStepCallBack(CrowdSimRenderer             *renderer,
                       std::queue<AgentGenerator *> &generatingList,
                       bool                         &isSimulating,
                       float                         stepInterval):
		NodeCallback(),
		_simRenderer(renderer), _simulating(isSimulating), _generatingList(generatingList), _stepInterval(stepInterval / 1000)
  {
  }

  virtual void  operator()(osg::Node *node, osg::NodeVisitor *nv)
	{
    double  currentTime = nv->getFrameStamp()->getSimulationTime();

		if (_lastTime < 0)
		{
			_lastTime = currentTime;
		}

		if (_simulating)
		{
      int  waitingListSize = _generatingList.size();

			if (waitingListSize > 0)
			{
				for (int i = 0; i < waitingListSize; i++)
				{
					_simRenderer->generateAgents(_generatingList.front());
					_generatingList.pop();
				}
			}
			else if (currentTime - _lastTime > _stepInterval)
			{
				_lastTime = currentTime;
				_simRenderer->nextStep();
			}
		}

		traverse(node, nv);
	}

  CrowdSimRenderer             *_simRenderer;
  std::queue<AgentGenerator *> &_generatingList;
  bool                         &_simulating;
  float                         _stepInterval;
  double                        _lastTime = -1;
};

CrowdSimRenderer::CrowdSimRenderer(osg::ref_ptr<osg::Node> model):
  QObject(), _currentAgentGroup(NULL), _targetRate(30),
	_simulator(NULL), _agentModel(model)
{
	qRegisterMetaType<osg::ref_ptr<osg::MatrixTransform>>();

	for (int i = 0; i < sizeof(_debugArrows) / sizeof(_debugArrows[0]); i++)
	{
		_debugArrows[i] = makeArrowIndicator(i);
	}
}

CrowdSimRenderer::~CrowdSimRenderer()
{
	pauseSimulation();
	resetSimulation();

	if (_cleanTimer)
  {
    delete _cleanTimer;
  }

	if (_simulator)
  {
    delete _simulator;
  }
}

void  CrowdSimRenderer::initSimulation()
{
	this->setMatrix(osg::Matrix::translate(osg::Vec3(0.0, 0.0, 8.1)));

	if (!_simulator)
	{
		_simulator = new CrowdSim(_config);
	}

	initTimer();
}

void  CrowdSimRenderer::initRenderRoot()
{
  // this->setDataVariance(Object::DYNAMIC);
}

osg::ref_ptr<osg::PositionAttitudeTransform>  CrowdSimRenderer::makeArrowIndicator(int type)
{
  osg::ref_ptr<osg::Cylinder>  cylinder = new osg::Cylinder;
	cylinder->setCenter(osg::Vec3(0, 0, 0));
	cylinder->setHeight(1.0f);
	cylinder->setRadius(0.05f);

  osg::ref_ptr<osg::Cone>  cone = new osg::Cone;
	cone->setCenter(osg::Vec3(0, 0.0f, 0.6f));
	cone->setHeight(0.3f);
	cone->setRadius(0.2f);

  osg::ref_ptr<osg::Geode>          geod          = new osg::Geode;
  osg::ref_ptr<osg::ShapeDrawable>  cylinderShape = new osg::ShapeDrawable(cylinder);
  osg::ref_ptr<osg::ShapeDrawable>  coneShape     = new osg::ShapeDrawable(cone);

	switch (type)
	{
  case (0):
		cylinderShape->setColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
		coneShape->setColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
		break;
  case (1):
		cylinderShape->setColor(osg::Vec4(0.0, 1.0, 0.0, 1.0));
		coneShape->setColor(osg::Vec4(0.0, 1.0, 0.0, 1.0));
		break;
  case (2):
		cylinderShape->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
		coneShape->setColor(osg::Vec4(0.0, 0.0, 1.0, 1.0));
		break;
	default:
		break;
	}

	geod->addDrawable(cylinderShape);
	geod->addDrawable(coneShape);

  osg::ref_ptr<osg::PositionAttitudeTransform>  arrow = new osg::PositionAttitudeTransform;
	arrow->setAttitude(osg::Quat(osg::DegreesToRadians(90.0f), osg::Vec3(0, 1, 0)));
	arrow->setPosition(osg::Vec3(0, 0, 0.5f));
	arrow->addChild(geod);

	return arrow;
}

void  CrowdSimRenderer::visualizeVector(AgentInstance &agent, osg::Vec3 vec, int mode, int index, float minSize)
{
  static const osg::Vec3  xAxis = osg::Vec3(1, 0, 0);
  static const osg::Vec3  zAxis = osg::Vec3(0, 0, 1);
  float                   size  = vec.length();

	if (size < minSize)
  {
    return;
  }

	vec.normalize();

  float  cosine   = vec * xAxis;
  float  rotation = osg::inRadians(acos(cosine));

	if (vec.y() < 0)
  {
    rotation = -rotation;
  }

  osg::Node *node = agent.instance->getChild(mode);

	if (mode == 1)
  {
    node = node->asSwitch()->getChild(index);
  }

  if (node)
	{
    osg::Transform *transform = node->asTransform();

		if (transform)
		{
      osg::PositionAttitudeTransform *pat = transform->asPositionAttitudeTransform();

			if (pat)
			{
				pat->setAttitude(osg::Quat(rotation, zAxis));

				if (index != 0)
				{
					pat->setScale(osg::Vec3(size, 1, 1));
				}
			}
			else
      {
        osg::notify(osg::NotifySeverity::WARN) << "PAT failed" << std::endl;
      }
    }
		else
    {
      osg::notify(osg::NotifySeverity::WARN) << "Transform failed" << std::endl;
    }
  }
	else
  {
    osg::notify(osg::NotifySeverity::WARN) << "Node failed" << std::endl;
  }
}

void  CrowdSimRenderer::addWayPoint(float x, float y, float r)
{
	try
	{
		_simulator->addWaypoint(_currentAgentGroup, x, y, r);
	}
  catch (exception &e)
	{
    emit  pathNotFound();
  }
}

void  CrowdSimRenderer::addObstacle(float x1, float y1, float x2, float y2)
{
	_simulator->addObstacle(x1, y1, x2, y2);
}

void  CrowdSimRenderer::addAgentGroup(float generateRate, float x, float y, float width, float height)
{
	_currentAgentGroup = _simulator->addAgentGroup(_config.generateNumber, x, y, width, height);

  osg::ref_ptr<osg::Group>  instanceGroup = new osg::Group;

  AgentGenerator *agentSource = new AgentGenerator {
    _currentAgentGroup, instanceGroup, x, y, width, height, new QTimer(), generateRate, _config.generateNumber
  };
	this->addChild(instanceGroup);

  // generateAgents(agentSource);

  connect(agentSource->generateTimer, &QTimer::timeout, [this, agentSource]()
  {
    _generatingList.push(agentSource);
  });

	_sources.push_back(agentSource);
}

void  CrowdSimRenderer::updatePath(Ped::AgentGroup *agentGroup, float xStart, float yStart, float xEnd, float yEnd)
{
	_simulator->findPath(agentGroup, xStart, yStart, xEnd, yEnd);
}

void  CrowdSimRenderer::setDebugMode(bool enable)
{
	_debugMode = enable;

  for (AgentInstance agent : _agents)
	{
		if (enable)
    {
      agent.instance->getChild(1)->asSwitch()->setAllChildrenOn();
    }
    else
    {
      agent.instance->getChild(1)->asSwitch()->setAllChildrenOff();
    }
  }
}

void  CrowdSimRenderer::generateAgents(AgentGenerator *source)
{
  auto  newAgents = _simulator->addAgents(
		source->group, source->generateNumber,
		source->x, source->y,
		source->width, source->height);

  // qDebug() << "Generated " << source->generateNumber << " agents.";
  // qDebug() << "Agent init timeline: ";

  for (const Ped::Agent *agent : newAgents)
	{
    osg::ref_ptr<osg::PositionAttitudeTransform>  node = new osg::PositionAttitudeTransform;
    // node->setPosition(fromTvector(agent->getPosition()));
		node->addChild(wrapPositionAttitude(_agentModel));

    osg::ref_ptr<osg::Switch>  debugNode = new osg::Switch;

		for (int i = 0; i < sizeof(_debugArrows) / sizeof(_debugArrows[0]); i++)
		{
			debugNode->addChild(wrapPositionAttitude(_debugArrows[i]));
		}

		if (_debugMode)
    {
      debugNode->setAllChildrenOn();
    }
    else
    {
      debugNode->setAllChildrenOff();
    }

    node->addChild(debugNode);

		node->setDataVariance(Object::DYNAMIC);
		source->intanceGroup->addChild(node);
    _agents.push_back(AgentInstance { node, agent });
    // qDebug() << agent->getTimeline();
	}
}

void  CrowdSimRenderer::nextStep()
{
	_simulator->updateScene();
	_simulator->moveAllAgents();

  for (AgentInstance agent : _agents)
	{
		agent.instance->setPosition(fromTvector(agent.agent->getPosition()));

		visualizeVector(agent, fromTvector(agent.agent->getVelocity()), 0, 0, 0);

		if (_debugMode)
		{
			visualizeVector(agent, fromTvector(agent.agent->getDesiredForce()), 1, 0, 0);
			visualizeVector(agent, fromTvector(agent.agent->getSocialForce()), 1, 1, 0);
			visualizeVector(agent, fromTvector(agent.agent->getObstacleForce()), 1, 2, 0);
		}
	}

  // auto debugPos = _agents[0].instance->getPosition();
  // qDebug() << "First agent pos: " << debugPos.x() << ", " << debugPos.y();

  emit  setAgentCount(_agents.size());
}

void  CrowdSimRenderer::cleanUp()
{
	_simulator->cleanUp();
}

void  CrowdSimRenderer::initTimer()
{
	_cleanTimer = new QTimer;

	QObject::connect(_cleanTimer, SIGNAL(timeout()), this, SLOT(cleanUp()));
}

void  CrowdSimRenderer::startSimulation()
{
  osg::ref_ptr<RendererStepCallBack>  updateCallBack = new RendererStepCallBack(
		this, _generatingList, _isSimulating, 1000 / _targetRate);
	this->setUpdateCallback(updateCallBack);

  for (AgentGenerator *source : _sources)
	{
		source->generateTimer->start(1000 / source->generateRate);
	}

	_cleanTimer->start(1000);

	_isSimulating = true;
}

void  CrowdSimRenderer::pauseSimulation()
{
  for (AgentGenerator *source : _sources)
	{
		source->generateTimer->stop();
	}

	_cleanTimer->stop();

	_isSimulating = false;
}

void  CrowdSimRenderer::resetSimulation()
{
	if (_simulator)
	{
		_simulator->init(_upperLeft.x(), _upperLeft.y(), _range.x(), _range.y());
		_simulator->reset();

		_agents.clear();

    for (AgentGenerator *source : _sources)
		{
			delete source->generateTimer;
		}

		_sources.clear();
	}
}

void  CrowdSimRenderer::resetPather()
{
	_simulator->resetPather();
}
