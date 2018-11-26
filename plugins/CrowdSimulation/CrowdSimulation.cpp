#include "CrowdSimulation.h"

#include <omp.h>
#include <cmath>

#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QSizePolicy>

#include <osg/Shape>
#include <osg/ComputeBoundsVisitor>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <AtlasMainWindow/AtlasMainWindow.h>
#include <AtlasMainWindow/NXDockWidget.h>
#include "CrowdSimRenderer.h"
#include <CrowdSim/PathPlanner.h>
#include <CrowdSim/cell.h>
#include <CrowdSim/grid.h>

#define MIN(a, b) a < b ? a : b

#define DEBUG_CELL
#ifdef DEBUG_CELL
static osg::ref_ptr<osg::Vec3Array> colors[10];
#endif

inline osg::ref_ptr<osg::Geode>  arrayToNode(osg::ref_ptr<osg::Vec3Array> array)
{
  osg::ref_ptr<osg::Geode>     geode = new osg::Geode;
  osg::ref_ptr<osg::Geometry>  geom  = new osg::Geometry;
  geom->setVertexArray((osg::Vec3Array *)array->clone(osg::CopyOp::DEEP_COPY_ALL));
	geode->addDrawable(geom);

	return geode;
}

inline osg::ref_ptr<osg::Vec3Array>  nodeToArray(osg::ref_ptr<osg::Node> node)
{
  return (osg::Vec3Array *)node->asGeode()->getDrawable(0)->asGeometry()->getVertexArray();
}

CrowdSimulation::CrowdSimulation():
	_numObstacle(0), _numPath(0), _numAgentGroup(0)
{
  _pluginName     = tr("Crowd Simulation");
	_pluginCategory = "Analysis";

	initSimulator(loadModel());
	initControlPanel();

	// Init handler
	_pickedPoints = new osg::Vec3Array;

#ifdef DEBUG_CELL

	for (int i = 0; i < 10; i++)
	{
		colors[i] = new osg::Vec3Array;
		colors[i]->push_back(osg::Vec3(i * 0.1f, (9 - i) * 0.1f, 0.0f));
	}

#endif
}

CrowdSimulation::~CrowdSimulation()
{
	delete _simRenderer;
}

void  CrowdSimulation::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("crowdSimulationAction"));
	_action->setCheckable(true);
  QIcon  icon49;
	icon49.addFile(QStringLiteral("resources/icons/crowd.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon49);
	_action->setText(tr("Crowd"));
	_action->setToolTip(tr("Crowd Simulation"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void  CrowdSimulation::initStage(osg::ref_ptr<osg::Node> baseBound, osg::ref_ptr<osg::Vec3Array> edgePoints, bool addToScene)
{
	// Get the base area
  osg::ComputeBoundsVisitor     visitor;
  osg::ref_ptr<osg::Transform>  transform = new osg::Transform;
	transform->addChild(baseBound);
	visitor.apply(*transform.get());
  osg::BoundingBox  bounding = visitor.getBoundingBox();

	// Get base size and origin position
  osg::Vec2  size = osg::Vec2(bounding.xMax() - bounding.xMin(), bounding.yMax() - bounding.yMin());
	size *= 1.05;
  osg::Vec2  center    = osg::Vec2(bounding.center().x(), bounding.center().y());
  osg::Vec2  upperLeft = center - size / 2;

	// Setup simulator
	qDebug() << "Checked crowd sim, upper-left: " << upperLeft.x() << ", " << upperLeft.y();
	qDebug() << "bottom-down: " << upperLeft.x() + size.x() << ", " << upperLeft.y() + size.y();
	resetSimulator(upperLeft, size);

	// Register all outer edges to the simulator
	for (unsigned int i = 1; i <= edgePoints->size(); i++)
	{
    const osg::Vec3 &thisPoint = edgePoints->at(i % edgePoints->size());
    const osg::Vec3 &lastPoint = edgePoints->at(i - 1);
    emit             addObstacle(thisPoint.x(), thisPoint.y(), lastPoint.x(), lastPoint.y());
	}

	if (addToScene)
	{
		_currentAnchor->addChild(baseBound);
	}

	recordNode(baseBound, QString("bounding"));

#ifdef DEBUG_CELL
	// Setup a cellmap for debug use
	_currentAnchor->removeChild(_cellMapRoot);
	_cellMapRoot = new osg::Geode;
	_currentAnchor->addChild(_cellMapRoot);

  auto  cells = _simRenderer->getScene()->getGrid()->getCells();

  for (const auto &row : cells)
	{
    for (const auto cell : row)
		{
      osg::ref_ptr<osg::Geometry>  cellGeom = generateCellGeom(cell);
			cellGeom->setDataVariance(Object::DYNAMIC);

			if (cell->attribute().passable)
      {
        _cellMapRoot->addDrawable(cellGeom);
      }

      _cellMap[cell] = cellGeom;
		}
	}

	recordNode(_cellMapRoot, QString("Cell Map"));
  emit    switchData(QString("Cell Map"), false);
  QTimer *timer = new QTimer;
  connect(timer, &QTimer::timeout, [this]()
  {
    this->updateCellMap();
  });
	timer->start(1000);
#endif

	_settingsToSave->addChild(baseBound);

	_controlPanel->setEnabled(true);
}

void  CrowdSimulation::initSimulator(osg::ref_ptr<osg::Node> model)
{
	// Init simulator
  _simRenderer    = new CrowdSimRenderer(model);
	_settingsToSave = new osg::Group;

	// Init simulator connections
	// TODO: Multi-thread implementation

  // _simRenderer->moveToThread(&_renderThread);
	_simRenderer->initSimulation();

  // connect(&_renderThread, SIGNAL(started()), _simRenderer, SLOT(initSimulation()));
	connect(this, SIGNAL(startSim()), _simRenderer, SLOT(startSimulation()));
	connect(this, SIGNAL(pauseSim()), _simRenderer, SLOT(pauseSimulation()));
	connect(this, SIGNAL(resetSim()), _simRenderer, SLOT(resetSimulation()));

  connect(this, SIGNAL(addWayPoint(float,float,float)), _simRenderer, SLOT(addWayPoint(float,float,float)));
  connect(this, SIGNAL(addObstacle(float,float,float,float)), _simRenderer, SLOT(addObstacle(float,float,float,float)));
  connect(this, SIGNAL(addAgentGroup(float,float,float,float,float)),
          _simRenderer, SLOT(addAgentGroup(float,float,float,float,float)));
  connect(this, SIGNAL(updatePath(Ped::AgentGroup *,float,float,float,float)),
          _simRenderer, SLOT(updatePath(Ped::AgentGroup *,float,float,float,float)));
  // connect(this, SIGNAL(setRenderRoot(osg::ref_ptr<osg::MatrixTransform>)),
  // _simRenderer, SLOT(setRenderRoot(osg::ref_ptr<osg::MatrixTransform>)));

	connect(_simRenderer, SIGNAL(setAgentCount(int)), this, SLOT(setAgentCount(int)));
	connect(_simRenderer, SIGNAL(pathNotFound()), this, SLOT(pathPlanFailed()));

  // _renderThread.start();
}

void  CrowdSimulation::registerObstacle(osg::ref_ptr<osg::Node> node, osg::ref_ptr<osg::Vec3Array> edgePoints, bool addToScene)
{
	// Register all obstacles to the simulator
	for (unsigned int i = 1; i <= edgePoints->size(); i++)
	{
    const osg::Vec3 &thisPoint = edgePoints->at(i % edgePoints->size());
    const osg::Vec3 &lastPoint = edgePoints->at(i - 1);
    emit             addObstacle(thisPoint.x(), thisPoint.y(), lastPoint.x(), lastPoint.y());
	}

  // emit addObstacle(_anchoredWorldPos.x(), _anchoredWorldPos.y(), _startPoint.x(), _startPoint.y());

#ifdef DEBUG_CELL
	updateCellMap();
#endif

	if (addToScene)
	{
		_currentAnchor->addChild(node);
	}

	recordNode(node, QString("obstacle: %1").arg(_numObstacle));

	_settingsToSave->addChild(node);

	_numObstacle++;
}

osg::ref_ptr<osg::Node>  CrowdSimulation::loadModel()
{
	// Load a model as agent
  osg::ref_ptr<osg::Node>  data = osgDB::readNodeFile("resources/models/man.osgb");

  float  zoomRate = 1.8f / (data->getBound().radius() * 2);

  osg::ref_ptr<osg::PositionAttitudeTransform>  model = new osg::PositionAttitudeTransform;
	model->addChild(data);
	model->setPosition(osg::Vec3(0, 0, data->getBound().radius()) * zoomRate);
	model->setScale(osg::Vec3(zoomRate, zoomRate, zoomRate));
  // model->setDataVariance(Object::DYNAMIC);

	return model;
}

void  CrowdSimulation::onLeftButton()
{
	if (_pluginRoot->getNumChildren() == 0)
	{
    // _renderRoot = new osg::MatrixTransform();
    // _renderRoot->setMatrix(osg::Matrix::translate(osg::Vec3(0.0, 0.0, 8.1)));
    // _currentAnchor->addChild(_renderRoot);
    // emit setRenderRoot(_renderRoot);

		_currentAnchor->addChild(_simRenderer);
	}

	switch (_mode)
	{
	case BASE:
		_style.lineColor = osg::Vec4(1.0, 0.0, 1.0, 1.0);
		pushObstacle();
		break;
	case OBSTACLE:
    _style.lineColor = osg::Vec4(0.0, 1.0, 1.0, 1.0);
		pushObstacle();
		break;
	case PATH:

		if (!_isDrawing)
		{
      emit  addAgentGroup(0.2, _anchoredWorldPos.x(), _anchoredWorldPos.y(), 5, 5);
		}

    _style.lineColor = osg::Vec4(0.0, 1.0, 0.0, 1.0);
		pushPath();
		break;
	default:
		break;
	}
}

void  CrowdSimulation::onDoubleClick()
{
	if (_isDrawing)
	{
		endDrawing();
		// Start wiith a point
		_currentDrawNode->addDrawable(createPointGeode(_endPoint, _intersections.begin()->getLocalIntersectNormal()));

    switch (_mode)
    {
		case BASE:
		{
			// Init the simulator stage based on the area bounding
			initStage(_currentDrawNode, _pickedPoints, false);
		}
		break;
		case OBSTACLE:
			registerObstacle(_currentDrawNode, _pickedPoints, false);
			break;
		case PATH:
		{
      for (const osg::Vec3 &pickedPoints : *_pickedPoints)
			{
        emit  addWayPoint(pickedPoints.x(), pickedPoints.y(), 5);
			}

			pushAgentSource();

			recordNode(_currentDrawNode, QString("agent path: %1").arg(_numPath));
			_numPath++;

			_setObstaclePushButton->setEnabled(false);
			_startSimulatingPushButton->setEnabled(true);
			_stopSimulatingPushButton->setEnabled(true);
		}
		break;
		default:
			break;
		}

		setNoneMode();
	}
}

void  CrowdSimulation::onMouseMove()
{
  switch (_mode)
  {
	case BASE:
	case OBSTACLE:

		if (_isDrawing)
		{
			// Draw a line to follow the mouse
			_endPoint = _anchoredWorldPos;

			if (_lastPoint != _endPoint)
			{
				_slicer.setEndPoint(_endPoint);
				updateIntersectedLine();

				// Enclose the polygon
        osg::ref_ptr<osg::Geometry>  tempGeom = _lineGeom;
				_lineGeom = _lastLine;
				_slicer.setStartPoint(_startPoint);
				updateIntersectedLine();
				_slicer.setStartPoint(_lastPoint);
				_lineGeom = tempGeom;
			}
		}

		break;
	case PATH:

		if (_isDrawing)
		{
			_endPoint = _anchoredWorldPos;

			if (_lastPoint != _endPoint)
			{
        emit  updatePath(_simRenderer->getAgentSources().back()->group,
                         _lastPoint.x(), _lastPoint.y(), _endPoint.x(), _endPoint.y());

				drawPath();
			}
		}

		break;
	default:
		break;
	}
}

void  CrowdSimulation::onRightButton()
{
	if (_isDrawing)
	{
		if (_currentDrawNode.valid())
		{
			_currentAnchor->removeChild(_currentDrawNode);
			_currentDrawNode = NULL;
		}

    switch (_mode)
    {
		case BASE:
		case OBSTACLE:
			break;
		case PATH:
			_simRenderer->resetPather();
			_simRenderer->getAgentSources().pop_back();
			break;
		default:
			break;
		}

		endDrawing();
	}
}

void  CrowdSimulation::drawPath()
{
  auto  lastAgentSource = _simRenderer->getAgentSources().back();

  osg::ref_ptr<osg::Vec3Array>  vecArray = new osg::Vec3Array;

  for (Ped::Cell *node : lastAgentSource->group->currentPath)
	{
    Ped::Tvector  center = node->center();
		vecArray->push_back(osg::Vec3(center.x, center.y, 8.5f));
	}

	_lineGeom->setVertexArray(vecArray);

  int  countitem = _lineGeom->getNumPrimitiveSets();

	for (int i = 0; i < countitem; i++)
	{
		_lineGeom->removePrimitiveSet(i);
	}

	_lineGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vecArray->size()));
	_lineGeom->dirtyDisplayList();
}

void  CrowdSimulation::pushPath()
{
	if (!_isDrawing)
	{
		beginDrawing();
		_startPoint = _anchoredWorldPos;
    _lastPoint  = _startPoint;

		_pickedPoints->clear();

		_currentDrawNode = new osg::Geode();
    osg::StateSet *state = _currentDrawNode->getOrCreateStateSet();
		state->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
		state->setRenderBinDetails(2, "RenderBin");

		_currentAnchor->addChild(_currentDrawNode);

		_slicerPointList = new osg::Vec3Array;
	}
	else
	{
		_lastPoint = _anchoredWorldPos;
	}

	_lineGeom = newLine();
	_currentDrawNode->addDrawable(_lineGeom.get());
	_currentDrawNode->addDrawable(createPointGeode(_lastPoint, _intersections.begin()->getLocalIntersectNormal()));

	_pickedPoints->push_back(_anchoredWorldPos);
}

void  CrowdSimulation::pushObstacle()
{
	if (!_isDrawing)
	{
		beginDrawing();
		_startPoint = _anchoredWorldPos;
    _lastPoint  = _startPoint;

		_pickedPoints->clear();

		// Init geode and save the whole path
		_currentDrawNode = new osg::Geode();
		_currentAnchor->addChild(_currentDrawNode);

		_slicerPointList = new osg::Vec3Array;

		// Init the last line segment for enclosing
		_lastLine = newLine();
		_currentDrawNode->addDrawable(_lastLine.get());
	}
	else
	{
		_lastPoint = _anchoredWorldPos;
	}

	// Begin drawing
	_lineGeom = newLine();
	_currentDrawNode->addDrawable(_lineGeom.get());
	_slicer.setStartPoint(_lastPoint);

	// Start with a point
	_currentDrawNode->addDrawable(createPointGeode(_lastPoint, _intersections.begin()->getLocalIntersectNormal()));
	_pickedPoints->push_back(_anchoredWorldPos);
}

void  CrowdSimulation::pushAgentSource()
{
  osg::ref_ptr<osg::Node>  nodeToRecord = _simRenderer->getAgentSources().back()->intanceGroup;
	recordNode(nodeToRecord, QString("agent group: %1").arg(_numAgentGroup));
	_numAgentGroup++;
}

void  CrowdSimulation::initControlPanel()
{
  QWidget     *panel;
	QVBoxLayout *mainLayout;
	QGridLayout *buttonGrid;
	QPushButton *setPathPushButton;
	QPushButton *loadSimButton;
	QPushButton *saveSimButton;
  QGroupBox   *instructionBox;
	QVBoxLayout *instructionLayout;
  QLabel      *insLabel_0;
  QLabel      *insLabel_1;
  QLabel      *insLabel_2;
	QGridLayout *parameterLayout;
  QLabel      *simRateTitle;
  QSlider     *simRateSlider;
  QLabel      *densityTitle;
  QSlider     *densitySlider;
  QLabel      *distanceTitle;
  QSlider     *distanceSlider;
	QHBoxLayout *debugLayout;
  QCheckBox   *simDebugCheckBox;
	QHBoxLayout *ROILayout;
	QPushButton *countRegionButton;
  QLabel      *ROILaybel;
	QHBoxLayout *controlLayout;
	QSpacerItem *verticalSpacer_5;

	_controlPanel = new NXDockWidget(tr("Crowd Simulation"), _mainWindow);
	_controlPanel->setObjectName(QStringLiteral("crowdSimController"));
	_controlPanel->setEnabled(true);
	_controlPanel->setFloating(false);
	_controlPanel->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
	_controlPanel->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	panel = new QWidget();
	panel->setObjectName(QStringLiteral("mainPanel"));
	mainLayout = new QVBoxLayout(panel);
	mainLayout->setSpacing(6);
	mainLayout->setContentsMargins(11, 11, 11, 11);
	mainLayout->setObjectName(QStringLiteral("mainLayout"));

	// Function buttons
	{
		buttonGrid = new QGridLayout();
		buttonGrid->setSpacing(6);
		buttonGrid->setObjectName(QStringLiteral("buttonGrid"));
		_setObstaclePushButton = new QPushButton(panel);
		_setObstaclePushButton->setObjectName(QStringLiteral("setObstaclePushButton"));
		_setObstaclePushButton->setText(tr("Add Obstacles"));

		buttonGrid->addWidget(_setObstaclePushButton, 1, 0, 1, 1);

		setPathPushButton = new QPushButton(panel);
		setPathPushButton->setObjectName(QStringLiteral("setPathPushButton"));
		setPathPushButton->setText(tr("Add Agents"));

		buttonGrid->addWidget(setPathPushButton, 1, 1, 1, 1);

		loadSimButton = new QPushButton(panel);
		loadSimButton->setObjectName(QStringLiteral("loadSimButton"));
		loadSimButton->setText(tr("Load"));

		buttonGrid->addWidget(loadSimButton, 0, 0, 1, 1);

		saveSimButton = new QPushButton(panel);
		saveSimButton->setObjectName(QStringLiteral("saveSimButton"));
		saveSimButton->setText(tr("Save"));

		buttonGrid->addWidget(saveSimButton, 0, 1, 1, 1);

		mainLayout->addLayout(buttonGrid);
	}

	// Instructions
	{
		instructionBox = new QGroupBox(panel);
		instructionBox->setObjectName(QStringLiteral("instructionBox"));
		instructionBox->setTitle(tr("Instructions"));
		instructionLayout = new QVBoxLayout(instructionBox);
		instructionLayout->setSpacing(6);
		instructionLayout->setContentsMargins(11, 11, 11, 11);
		instructionLayout->setObjectName(QStringLiteral("instructionLayout"));
		insLabel_0 = new QLabel(instructionBox);
		insLabel_0->setObjectName(QStringLiteral("insLabel_0"));
		insLabel_0->setMargin(3);
		insLabel_0->setText(tr("1. Add obstacles to the scene."));

		instructionLayout->addWidget(insLabel_0);

		insLabel_1 = new QLabel(instructionBox);
		insLabel_1->setObjectName(QStringLiteral("insLabel_1"));
		insLabel_1->setMargin(3);
		insLabel_1->setText(tr("2. Add agents and their desired path."));

		instructionLayout->addWidget(insLabel_1);

		insLabel_2 = new QLabel(instructionBox);
		insLabel_2->setObjectName(QStringLiteral("insLabel_2"));
		insLabel_2->setMargin(3);
		insLabel_2->setText(tr("3. Start simulation."));

		instructionLayout->addWidget(insLabel_2);

		mainLayout->addWidget(instructionBox);
	}

	// Parameter sliders
	{
		parameterLayout = new QGridLayout();
		parameterLayout->setSpacing(6);
		parameterLayout->setObjectName(QStringLiteral("parameterLayout"));

		// Sim rate
		simRateTitle = new QLabel(panel);
		simRateTitle->setObjectName(QStringLiteral("simRateTitle"));
		simRateTitle->setText(tr("Rate"));

		parameterLayout->addWidget(simRateTitle, 0, 0);

		_simRateLabel = new QLabel(panel);
		_simRateLabel->setObjectName(QStringLiteral("simRateLabel"));
    QSizePolicy  sizePolicy6(QSizePolicy::Fixed, QSizePolicy::Preferred);
		sizePolicy6.setHorizontalStretch(0);
		sizePolicy6.setVerticalStretch(0);
		sizePolicy6.setHeightForWidth(_simRateLabel->sizePolicy().hasHeightForWidth());
		_simRateLabel->setSizePolicy(sizePolicy6);
		_simRateLabel->setMinimumSize(QSize(30, 0));
		_simRateLabel->setMaximumSize(QSize(30, 16777215));
		_simRateLabel->setAlignment(Qt::AlignCenter);

		parameterLayout->addWidget(_simRateLabel, 0, 1);

		simRateSlider = new QSlider(panel);
		simRateSlider->setObjectName(QStringLiteral("simRateSlider"));
		simRateSlider->setOrientation(Qt::Horizontal);

		parameterLayout->addWidget(simRateSlider, 0, 2);

		// Density
		densityTitle = new QLabel(panel);
		densityTitle->setObjectName(QStringLiteral("densityTitle"));
		densityTitle->setText(tr("Density"));

		parameterLayout->addWidget(densityTitle, 1, 0);

		_densityLabel = new QLabel(panel);
		_densityLabel->setObjectName(QStringLiteral("densityLabel"));
		sizePolicy6.setHeightForWidth(_densityLabel->sizePolicy().hasHeightForWidth());
		_densityLabel->setSizePolicy(sizePolicy6);
		_densityLabel->setMinimumSize(QSize(30, 0));
		_densityLabel->setMaximumSize(QSize(30, 16777215));
		_densityLabel->setAlignment(Qt::AlignCenter);

		parameterLayout->addWidget(_densityLabel, 1, 1);

		densitySlider = new QSlider(panel);
		densitySlider->setObjectName(QStringLiteral("densitySlider"));
		densitySlider->setOrientation(Qt::Horizontal);

		parameterLayout->addWidget(densitySlider, 1, 2);

		// Distance
		distanceTitle = new QLabel(panel);
		distanceTitle->setObjectName(QStringLiteral("distanceTitle"));
		distanceTitle->setText(tr("Distance"));

		parameterLayout->addWidget(distanceTitle, 2, 0);

		_distanceLabel = new QLabel(panel);
		_distanceLabel->setObjectName(QStringLiteral("distanceLabel"));
		sizePolicy6.setHeightForWidth(_distanceLabel->sizePolicy().hasHeightForWidth());
		_distanceLabel->setSizePolicy(sizePolicy6);
		_distanceLabel->setMinimumSize(QSize(30, 0));
		_distanceLabel->setMaximumSize(QSize(30, 16777215));
		_distanceLabel->setAlignment(Qt::AlignCenter);

		parameterLayout->addWidget(_distanceLabel, 2, 1);

		distanceSlider = new QSlider(panel);
		distanceSlider->setObjectName(QStringLiteral("distanceSlider"));
		distanceSlider->setOrientation(Qt::Horizontal);

		parameterLayout->addWidget(distanceSlider, 2, 2);

		mainLayout->addLayout(parameterLayout);
	}

	// Debug
	debugLayout = new QHBoxLayout();
	debugLayout->setSpacing(6);
	debugLayout->setObjectName(QStringLiteral("debugLayout"));
	simDebugCheckBox = new QCheckBox(panel);
	simDebugCheckBox->setObjectName(QStringLiteral("simDebugCheckBox"));
	simDebugCheckBox->setText(tr("Debug Mode"));

	debugLayout->addWidget(simDebugCheckBox);

	mainLayout->addLayout(debugLayout);

	// ROI
	ROILayout = new QHBoxLayout();
	ROILayout->setSpacing(6);
	ROILayout->setObjectName(QStringLiteral("ROILayout"));
	countRegionButton = new QPushButton(panel);
	countRegionButton->setObjectName(QStringLiteral("countRegionButton"));
	countRegionButton->setText(tr("Set ROI"));

	ROILayout->addWidget(countRegionButton);

	ROILaybel = new QLabel(panel);
	ROILaybel->setObjectName(QStringLiteral("ROILaybel"));
	ROILaybel->setAlignment(Qt::AlignCenter);
	ROILaybel->setText(tr("Agents in Region:"));

	ROILayout->addWidget(ROILaybel);

	_agentCountLabel = new QLabel(panel);
	_agentCountLabel->setObjectName(QStringLiteral("agentCountLabel"));
  QSizePolicy  sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(_agentCountLabel->sizePolicy().hasHeightForWidth());
	_agentCountLabel->setSizePolicy(sizePolicy1);
	_agentCountLabel->setMinimumSize(QSize(0, 0));
	_agentCountLabel->setAlignment(Qt::AlignCenter);

	ROILayout->addWidget(_agentCountLabel);

	mainLayout->addLayout(ROILayout);

	// Control
	{
		controlLayout = new QHBoxLayout();
		controlLayout->setSpacing(6);
		controlLayout->setObjectName(QStringLiteral("controlLayout"));
		_resetSimulatingPushButton = new QPushButton(panel);
		_resetSimulatingPushButton->setObjectName(QStringLiteral("resetSimulatingPushButton"));
		_resetSimulatingPushButton->setText(tr("Reset"));

		controlLayout->addWidget(_resetSimulatingPushButton);

		_startSimulatingPushButton = new QPushButton(panel);
		_startSimulatingPushButton->setObjectName(QStringLiteral("startSimulatingPushButton"));
		_startSimulatingPushButton->setText(tr("Start"));

		controlLayout->addWidget(_startSimulatingPushButton);

		_stopSimulatingPushButton = new QPushButton(panel);
		_stopSimulatingPushButton->setObjectName(QStringLiteral("stopSimulatingPushButton"));
		_stopSimulatingPushButton->setText(tr("Stop"));

		controlLayout->addWidget(_stopSimulatingPushButton);

		mainLayout->addLayout(controlLayout);
	}

	verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	mainLayout->addItem(verticalSpacer_5);

	// Add to main UI
	_controlPanel->setWidget(panel);
	_controlPanel->setHidden(true);
  (static_cast<AtlasMainWindow *>(_mainWindow))->addDockWidget(Qt::RightDockWidgetArea, _controlPanel);

	// Behavior of the control panel
	connect(_setObstaclePushButton, SIGNAL(clicked()), this, SLOT(setObstacleMode()));
	connect(setPathPushButton, SIGNAL(clicked()), this, SLOT(setPathMode()));
	connect(_startSimulatingPushButton, SIGNAL(clicked()), this, SIGNAL(startSim()));
	connect(_stopSimulatingPushButton, SIGNAL(clicked()), this, SIGNAL(pauseSim()));
	connect(_resetSimulatingPushButton, SIGNAL(clicked()), this, SLOT(onResetSimulatingPushButtonClicked()));
	connect(simDebugCheckBox, SIGNAL(stateChanged(int)), this, SLOT(onSimDebugCheckBoxStatusChanged(int)));
	connect(saveSimButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	connect(loadSimButton, SIGNAL(clicked()), this, SLOT(loadSettings()));

	// Init parameter sliders
  float  simRate, minRate, maxRate;
	_simRenderer->getSimRate(simRate, minRate, maxRate);
	simRateSlider->setValue((simRate - minRate) / (maxRate - minRate) * 99);
	_simRateRange[0] = minRate;
	_simRateRange[1] = maxRate;
	_simRateLabel->setText(QString::number(simRate));

  double  simMaxDensity, minMaxDensity, maxMaxDensity;
	_simRenderer->getSimDensity(simMaxDensity, minMaxDensity, maxMaxDensity);
	densitySlider->setValue(log(simMaxDensity / minMaxDensity) / log(maxMaxDensity / minMaxDensity) * 99);
	_simMaxDensity[0] = minMaxDensity;
	_simMaxDensity[1] = maxMaxDensity;
	_densityLabel->setText(QString::number(simMaxDensity));

  double  pedDist, minDist, maxDist;
	_simRenderer->getPedDistance(pedDist, minDist, maxDist);
	distanceSlider->setValue(log(pedDist / minDist) / log(maxDist / minDist) * 100);
	_pedDistRange[0] = minDist;
	_pedDistRange[1] = maxDist;
	_distanceLabel->setText(QString::number(pedDist));

	connect(densitySlider, SIGNAL(valueChanged(int)), _simRenderer, SLOT(setSimDensity(int)));
	connect(simRateSlider, SIGNAL(valueChanged(int)), _simRenderer, SLOT(setSimRate(int)));
	connect(distanceSlider, SIGNAL(valueChanged(int)), _simRenderer, SLOT(setPedDistance(int)));
}

osg::ref_ptr<osg::Geometry>  CrowdSimulation::generateCellGeom(const Ped::Cell *cell)
{
  osg::ref_ptr<osg::Geometry>  cellGeom = new osg::Geometry;

  osg::ref_ptr<osg::Vec3Array>  colorArray = new osg::Vec3Array;
	colorArray->push_back(osg::Vec3(0.5, 0.5, 0.5));
	cellGeom->setColorArray(colorArray);
	cellGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

  osg::Vec3                     topLeft(cell->x(), cell->y(), 8.0);
  Ped::Tvector                  size      = cell->size();
  osg::ref_ptr<osg::Vec3Array>  vertArray = new osg::Vec3Array;
	vertArray->push_back(topLeft);
	vertArray->push_back(topLeft + osg::Vec3(0, size.y, 0));
	vertArray->push_back(topLeft + osg::Vec3(size.x, size.y, 0));
	vertArray->push_back(topLeft + osg::Vec3(size.x, 0, 0));
	cellGeom->setVertexArray(vertArray);

	cellGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, vertArray->size()));

	return cellGeom;
}

void  CrowdSimulation::setDebugMode(bool enable)
{
	_debugOn = enable;
  emit  switchData(QString("cell map"), enable);

	_simRenderer->setDebugMode(enable);
}

void  CrowdSimulation::pathPlanFailed()
{
	QMessageBox::warning(0, tr("Error!"), tr("Cannot find a valid path, please try pick another point."));
	this->onRightButton();
}

void  CrowdSimulation::resetSimulator(osg::Vec2 upperLeft, osg::Vec2 size)
{
	_simRenderer->setBounding(upperLeft, size);
  emit  resetSim();
}

void  CrowdSimulation::resetSimulation()
{
  emit  pauseSim();

	setNoneMode();
  // emit resetSim();

	if (_simRenderer.valid())
	{
		_simRenderer->removeChildren(0, _simRenderer->getNumChildren());
	}

  _numObstacle   = 0;
  _numPath       = 0;
	_numAgentGroup = 0;

	_settingsToSave = new osg::Group;

  emit  removeData(_pluginName);
	onRightButton();
  // _subGroup = NULL;
}

void  CrowdSimulation::updateCellMap()
{
#ifdef DEBUG_CELL

	if (_debugOn)
	{
		for (auto cell : _cellMap.keys())
		{
      auto  node = _cellMap[cell];

			if (cell->attribute().passable == false)
			{
				_cellMapRoot->removeDrawable(node);
			}

			if (cell->changed())
			{
				_cellMapRoot->removeDrawable(node);
				node->asGeometry()->setColorArray(colors[MIN(cell->getAgents().size(), 9)]);
				node->asGeometry()->setColorBinding(osg::Geometry::BIND_OVERALL);
				_cellMapRoot->addDrawable(node);
				cell->changed() = false;
			}
		}
	}

#endif
}

osg::ref_ptr<osg::Vec3Array>  getContourNode(osg::ref_ptr<osg::Geode> geode)
{
  osg::ref_ptr<osg::Vec3Array>  nodes = new osg::Vec3Array;

	for (unsigned int i = 0; i < geode->getNumDrawables(); i++)
	{
    osg::Geometry *geom = geode->getDrawable(i)->asGeometry();

		if (geom)
		{
			if (geom->getName() == "point")
			{
        nodes->push_back((static_cast<const osg::Vec3Array *>(geom->getVertexArray()))->at(0));
			}
		}
	}

	return nodes;
}

bool  CrowdSimulation::loadSettings()
{
  QString  loadPath = QFileDialog::getOpenFileName(0, tr("Open file"), "",
                                                   tr("OSG file(*.osg *.osgb);; Allfile(*.*)"));

	if (loadPath.isEmpty())
  {
    return false;
  }

  osg::Node *nodeFile = osgDB::readNodeFile(loadPath.toLocal8Bit().toStdString());

  if (!nodeFile)
  {
    QMessageBox::warning(0, tr("Error"), tr("Loading failed"));

    return false;
  }

  osg::ref_ptr<osg::Group>  sceneSetting = nodeFile->asGroup();

  if (!sceneSetting.valid())
  {
    QMessageBox::warning(0, tr("Error"), tr("Loading failed"));

    return false;
  }

  _settingsToSave = new osg::Group;
  _currentAnchor->addChild(_simRenderer);

  try
  {
    // Restore base stage
    initStage(sceneSetting->getChild(0), getContourNode(sceneSetting->getChild(0)->asGeode()), true);

    // Restore obstacles
    for (unsigned int i = 1; i < sceneSetting->getNumChildren(); i++)
    {
      registerObstacle(sceneSetting->getChild(i), getContourNode(sceneSetting->getChild(i)->asGeode()), true);
    }
  }
  catch (std::exception e)
  {
    QMessageBox::critical(0, tr("Error"), tr("Invalid setting record."));

    return false;
  }

  // Re-add nodes to make sure all nodes' first parent is attached to scene roots
  for (unsigned int i = 0; i < sceneSetting->getNumChildren(); i++)
  {
    osg::ref_ptr<osg::Node>  child = sceneSetting->getChild(0);
    sceneSetting->removeChild(child);
    sceneSetting->addChild(child);
  }

  return true;
}

void  CrowdSimulation::saveSettings()
{
  QString  savePath = QFileDialog::getSaveFileName(0, tr("Save file"), "",
                                                   tr("OSG plain text(*.osg);; OSG binary(*.osgb);; Allfile(*.*)"));

	if (savePath.isEmpty())
  {
    return;
  }

  bool  writeResult = false;

	while (!writeResult)
	{
		writeResult = osgDB::writeNodeFile(*_settingsToSave, savePath.toLocal8Bit().toStdString());

		if (!writeResult)
		{
      int  result = QMessageBox::warning(0, tr("Error"), tr("Save failed, specify another position?"),
                                         QMessageBox::Ok | QMessageBox::Cancel);

			if (result == QMessageBox::Cancel)
			{
				writeResult = true;
			}
		}
	}
}

void  CrowdSimulation::toggle(bool checked)
{
  if (checked)
  {
    // osg::BoundingSphere bounding = _dataRoot->getBound();
    // osg::Vec2 size = osg::Vec2(bounding.radius(), bounding.radius());
    // osg::Vec2 offset = osg::Vec2(_mousePicker->getDrawOffset().x(), _mousePicker->getDrawOffset().y());
    // osg::Vec2 upperLeft = osg::Vec2(bounding.center().x(), bounding.center().y()) - size - offset;
		//
    // qDebug() << "Checked crowd sim, upper-left: " << upperLeft.x() << ", " << upperLeft.y();
    // qDebug() << "bottom-down: " << upperLeft.x() + size.x() * 2 << ", " << upperLeft.y() + size.y() * 2;

    // crowdSimAnalysis->resetSimulator(upperLeft, size * 2);
    int  mode = QMessageBox::question(0, tr("Choose mode."), tr("Load an existing setting?"), QDialogButtonBox::Yes, QDialogButtonBox::No);

		if (mode == QDialogButtonBox::Yes)
		{
			if (!loadSettings())
			{
				_action->setChecked(false);

				return;
			}
			else
      {
        setNoneMode();
      }
    }
		else
		{
			setAreaMode();

			_startSimulatingPushButton->setEnabled(false);
			_stopSimulatingPushButton->setEnabled(false);
			_controlPanel->setEnabled(false);
		}
	}
	else
	{
		resetSimulation();
	}

	_controlPanel->setVisible(checked);

	_activated = checked;
}

void  CrowdSimulation::crowdSimDebugMode(bool checked)
{
  emit  switchData(QString("Cell Map"), checked);

	setDebugMode(checked);
}

void  CrowdSimulation::setSimRate(int value)
{
  float  rate = (float)value / 99 * (_simRateRange[1] - _simRateRange[0]) + _simRateRange[0];

	_simRateLabel->setText(QString::number(rate, 'f', 1));
	_simRenderer->setSimRate(rate);
}

void  CrowdSimulation::setSimDensity(int value)
{
  double  density = exp((float)value / 99 * log(_simMaxDensity[1] / _simMaxDensity[0])) * _simMaxDensity[0];

	_densityLabel->setText(QString::number(density, 'f', 2));
	_simRenderer->setSimDensity(density);
}

void  CrowdSimulation::setPedDistance(int value)
{
  double  dist = exp((float)value / 99 * log(_pedDistRange[1] / _pedDistRange[0])) * _pedDistRange[0];

	_distanceLabel->setText(QString::number(dist, 'f', 2));
	_simRenderer->setPedDistance(dist);
}

void  CrowdSimulation::setAgentCount(int value)
{
	_agentCountLabel->setText(QString::number(value));
}

void  CrowdSimulation::onResetSimulatingPushButtonClicked()
{
	resetSimulation();

	_setObstaclePushButton->setEnabled(true);
	_startSimulatingPushButton->setEnabled(false);
	_stopSimulatingPushButton->setEnabled(false);
	_controlPanel->setEnabled(false);

	setAreaMode();
}

void  CrowdSimulation::onSimDebugCheckBoxStatusChanged(int status)
{
	setDebugMode(status == Qt::Checked);
}
