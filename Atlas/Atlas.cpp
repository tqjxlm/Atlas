#include "Atlas.h"

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QTreeWidgetItem>

#include <osg/Notify>
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>

#include <osg/BlendColor>
#include <osg/BlendFunc>
#include <osg/TexGen>
#include <osg/TexEnv>

#include <osgSim/OverlayNode>
#include <osgDB/FileUtils>
#include <osgEarth/Map>
#include <osgEarth/MapNode>

#include <gdal_priv.h>

#include <DataManager/DataManager.h>
#include <ViewerWidget/ViewerWidget.h>
#include <MousePicker/MousePicker.h>
#include <SettingsManager/SettingsManager.h>
#include <PluginManager/PluginManager.h>
#include <MapController/MapController.h>
#include "ui_AtlasMainWindow.h"

Atlas::Atlas(QWidget *parent, Qt::WindowFlags flags):
  AtlasMainWindow(parent, flags)
{
  // Some global environment settings
  QCoreApplication::setOrganizationName("Atlas");
  QCoreApplication::setApplicationName("Atlas");

  GDALAllRegister();
  CPLSetConfigOption("GDAL_DATA", ".\\resources\\GDAL_data");

  osg::DisplaySettings::instance()->setNumOfHttpDatabaseThreadsHint(4);
  osg::DisplaySettings::instance()->setNumOfDatabaseThreadsHint(2);
}

Atlas::~Atlas()
{
	delete _pluginManager;
	delete _dataManager;
	delete _settingsManager;

	cout << "Program closed: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str() << endl;
	_log->close();
	delete _log;

	osg::setNotifyLevel(osg::FATAL);
}

void  Atlas::initAll()
{
  collectInitInfo();

  emit  sendNowInitName(tr("Initializing log"));
	initLog();

  emit  sendNowInitName(tr("Initializing UI"));
	setupUi();

	initCore();

  initDataStructure();

  emit  sendNowInitName(tr("Initializing camera"));
	resetCamera();

	initPlugins();

  emit  sendNowInitName(tr("Stylizing UI"));
	initUiStyles();
}

void  Atlas::initCore()
{
  emit  sendNowInitName(tr("Initializing DataManager"));

  _root = new osg::Group;
	_root->setName("Root");

	_settingsManager = new SettingsManager();
  _settingsManager->setupUi(_ui->projectMenu);

	_dataManager = new DataManager(_settingsManager, this);

	connect(_dataManager, &DataManager::loadingProgress, this, &AtlasMainWindow::loadingProgress);
	connect(_dataManager, &DataManager::loadingDone, this, &AtlasMainWindow::loadingDone);
	connect(_dataManager, &DataManager::resetCamera, this, &Atlas::resetCamera);

  emit  sendNowInitName(tr("Initializing viewer"));
	_mainViewerWidget = new ViewerWidget(_root, 0, 0, 1280, 1024, osgViewer::ViewerBase::SingleThreaded);
	_mainViewerWidget->getMainView()->getCamera()->setCullMask(SHOW_IN_WINDOW_1 | SHOW_IN_NO_WINDOW);
  emit  sendNowInitName(tr("Initializing viewer"));
  setCentralWidget(_mainViewerWidget);

	connect(_dataManager, SIGNAL(startRendering()), _mainViewerWidget, SLOT(startRendering()));
	connect(_dataManager, SIGNAL(stopRendering()), _mainViewerWidget, SLOT(stopRendering()));

  _pluginManager = new PluginManager(this, _dataManager, _mainViewerWidget);
	_pluginManager->registerPluginGroup("Data", _ui->dataToolBar, _ui->dataMenu);
	_pluginManager->registerPluginGroup("Measure", _ui->measToolBar, _ui->measMenu);
	_pluginManager->registerPluginGroup("Draw", _ui->drawToolBar, _ui->drawMenu);
	_pluginManager->registerPluginGroup("Effect", _ui->effectToolBar, _ui->effectMenu);
	_pluginManager->registerPluginGroup("Analysis", _ui->analysisToolBar, _ui->analysisMenu);
	_pluginManager->registerPluginGroup("Edit", _ui->editToolBar, _ui->editMenu);

  connect(_dataManager, &DataManager::requestContextMenu, _pluginManager, &PluginManager::loadContextMenu);
  connect(_pluginManager, &PluginManager::sendNowInitName, this, &Atlas::sendNowInitName);
}

void  Atlas::initPlugins()
{
  // MousePicker is the shared core of all plugins
  _mousePicker = new MousePicker();
  _mousePicker->registerData(this, _dataManager, _mainViewerWidget, _root, _settingsManager->getGlobalSRS());
  _mousePicker->registerSetting(_settingsManager);
  _mainViewerWidget->getMainView()->addEventHandler(_mousePicker);
  connect(_mousePicker, SIGNAL(updateText1(QString)), _labelLocalCoord, SLOT(setText(QString)));
  connect(_mousePicker, SIGNAL(updateText2(QString)), _labelWorldCoord, SLOT(setText(QString)));
  connect(_mousePicker, SIGNAL(updateText3(QString)), _labelGeoCoord, SLOT(setText(QString)));

	_pluginManager->loadPlugins();
}

void  Atlas::initDataStructure()
{
  // Init data root to contain user data
  // It is different from draw root in that data root is intersectable and projectable
  _dataRoot = new osg::PositionAttitudeTransform;
  _dataRoot->setName("Data Root");

	// Init osgEarth node using the predefined .earth file
	for (int i = 0; i < MAX_SUBVIEW; i++)
	{
    osg::Node *baseMap = osgDB::readNodeFile("resources/earth_files/base.earth");
		_mapNode[i] = osgEarth::MapNode::get(baseMap);
		_mapNode[i]->setName(QString("Map%1").arg(i).toStdString());
		_mapNode[i]->setNodeMask(SHOW_IN_WINDOW_1 << i);
		_mapNode[i]->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

		_mainMap[i] = _mapNode[i]->getMap();
    _dataRoot->addChild(_mapNode[i]);
  }

	_settingsManager->setGlobalSRS(_mainMap[0]->getSRS());

  // Init overlayNode with overlayerSubgraph
  // Everything in overlaySubgraph will be projected to its children
  _overlayNode = new osgSim::OverlayNode(osgSim::OverlayNode::OBJECT_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY);
  _overlayNode->setName("World Overlay");
  _overlayNode->getOrCreateStateSet()->setAttributeAndModes(
    new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));

  _overlaySubgraph = new osg::PositionAttitudeTransform;
  _overlayNode->setOverlaySubgraph(_overlaySubgraph);
  _overlayNode->addChild(_dataRoot);

	// Root for all drawings
	_drawRoot = new osg::PositionAttitudeTransform;
	_drawRoot->setName("Draw Root");

  _root->addChild(_overlayNode);
  _root->addChild(_drawRoot);

  _dataManager->registerDataRoots(_root);
}

void  Atlas::resetCamera()
{
  MapController *manipulator = _mainViewerWidget->getManipulator();

  if (manipulator == NULL)
  {
    // Init a manipulator if not inited yet
    manipulator = new MapController(_dataRoot);
    _mainViewerWidget->getMainView()->setCameraManipulator(manipulator);
    manipulator->setAutoComputeHomePosition(false);

    manipulator->setCenterIndicator(_mainViewerWidget->createCameraIndicator());
  }

	_mainViewerWidget->resetCamera(_mapNode[0]);

  connect(_dataManager, SIGNAL(moveToNode(const osg::Node *,double)),
          manipulator, SLOT(fitViewOnNode(const osg::Node *,double)),
          Qt::UniqueConnection);
  connect(_dataManager, SIGNAL(moveToBounding(const osg::BoundingSphere *,double)),
          manipulator, SLOT(fitViewOnBounding(const osg::BoundingSphere *,double)),
          Qt::UniqueConnection);
}

void  qtLogToFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  const string  logTypes[] = { "Debug", "Warning", "Critical", "Fatal", "Info" };

	cout << "[Qt]    [" << logTypes[type] << "]    " << msg.toLocal8Bit().constData();
#ifndef NDEBUG
	cout << "    (" << context.file << ": " << context.line << ", " << context.function;
#endif
	cout << endl;
}

class LogFileHandler: public osg::NotifyHandler
{
  const std::string  severityTag[osg::DEBUG_FP + 1] = {
		"ALWAYS",
		"FATAL",
		"WARN",
		"NOTICE",
		"INFO",
		"DEBUG_INFO",
		"DEBUG_FP"
	};

public:
  LogFileHandler(std::ofstream *outStream):
    _log(outStream)
	{
	}

  virtual void  notify(osg::NotifySeverity severity, const char *msg)
	{
		if (_log)
		{
			(*_log) << "[osg]    " << "[" << severityTag[severity] << "]    " << msg;
			_log->flush();
		}
	}

	~LogFileHandler()
	{
	}

protected:
  std::ofstream *_log = NULL;
};

void  Atlas::initLog()
{
	// Open log file
#ifdef _WIN32
	const char *appData = getenv("APPDATA");
#else
	const char *appData = "/tmp";
#endif

  std::string  logDir = QString("%1/%2/%3")
                        .arg(appData)
                        .arg(QApplication::organizationName())
                        .arg(QApplication::applicationName())
                        .toStdString();

	if (!osgDB::fileExists(logDir))
  {
    osgDB::makeDirectory(logDir);
  }

  std::string  logPath = logDir + "/AtlasLog.txt";
	_log = NULL;
	_log = new std::ofstream(logPath.c_str());

	if (!_log)
  {
    return;
  }

	// Redirect std iostream
	std::cout.rdbuf(_log->rdbuf());
	std::cerr.rdbuf(_log->rdbuf());

	// Redirect qt logs to stdout, thus to our log file
	qInstallMessageHandler(qtLogToFile);

	// Redirect OSGEarth
	if (getenv("OSGEARTH_PACKAGE_LOGGING") != 0)
	{
    std::string  level(getenv("OSGEARTH_PACKAGE_LOGGING"));

		if (level == "INFO")
    {
      osgEarth::setNotifyLevel(osg::INFO);
    }
    else if (level == "DEBUG")
    {
      osgEarth::setNotifyLevel(osg::DEBUG_INFO);
    }
  }
	else
	{
		osgEarth::setNotifyLevel(osg::INFO);
	}

	// Redirect OSG
	osg::setNotifyLevel(osg::NOTICE);
	osg::setNotifyHandler(new LogFileHandler(_log));

	cout << "Program started: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str() << endl;
}

void  Atlas::setupUi()
{
  AtlasMainWindow::setupUi();
  connect(_ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
  connect(_ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
}

void  Atlas::collectInitInfo()
{
  // TODO: find a better way to collect initialization info
  int   initSteps = 7;
  QDir  pluginsDir(qApp->applicationDirPath());

  pluginsDir.cd("plugins");

  // Parsing plugin dependencies
  foreach(QString fileName, pluginsDir.entryList(QDir::Files))
  {
    if (fileName.split('.').back() != "dll")
    {
      continue;
    }

    initSteps++;
  }
  emit  sendTotalInitSteps(initSteps);
}

void  Atlas::about()
{
  QString  versionInfo(tr("Atlas v0.0.1 by tqjxlm"));

	QMessageBox::information(this, tr("version"), versionInfo);
}
