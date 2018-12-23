#include "Atlas.h"

#include <iostream>
#include <string>
#include <fstream>
using namespace std;

#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QProcess>
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
#include <osgEarth/Registry>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/LogarithmicDepthBuffer>
#include <osgEarthUtil/ExampleResources>

#include <gdal_priv.h>

#include <DataManager/DataManager.h>
#include <ViewerWidget/ViewerWidget.h>
#include <MousePicker/MousePicker.h>
#include <SettingsManager/SettingsManager.h>
#include <PluginManager/PluginManager.h>
#include <MapController/MapController.h>
#include <ui_AtlasMainWindow.h>

Atlas::Atlas(QWidget *parent, Qt::WindowFlags flags):
	AtlasMainWindow(parent, flags)
{
	// Some global environment settings
	QCoreApplication::setOrganizationName("Atlas");
	QCoreApplication::setApplicationName("Atlas");

	GDALAllRegister();
	CPLSetConfigOption("GDAL_DATA", ".\\resources\\GDAL_data");

	osg::DisplaySettings::instance()->setNumOfHttpDatabaseThreadsHint(8);
	osg::DisplaySettings::instance()->setNumOfDatabaseThreadsHint(2);
}

Atlas::~Atlas()
{
	delete _pluginManager;
	delete _dataManager;
	delete _settingsManager;

	osg::setNotifyLevel(osg::FATAL);
	osg::setNotifyHandler(nullptr);
	osgEarth::setNotifyHandler(nullptr);

	cout << "Program closed: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str() << endl;
	_log->close();
	delete _log;
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

	_settingsManager = new SettingsManager(this);
	_settingsManager->setupUi(_ui->projectMenu);

	_dataManager = new DataManager(_settingsManager, this);

	connect(_dataManager, &DataManager::loadingProgress, this, &AtlasMainWindow::loadingProgress);
	connect(_dataManager, &DataManager::loadingDone, this, &AtlasMainWindow::loadingDone);
	connect(_dataManager, &DataManager::resetCamera, this, &Atlas::resetCamera);

	emit  sendNowInitName(tr("Initializing viewer"));
	_mainViewerWidget = new ViewerWidget(_root, 0, 0, 1280, 1024, osgViewer::ViewerBase::SingleThreaded);
	_mainViewerWidget->getMainView()->getCamera()->setCullMask(SHOW_IN_WINDOW_1);
	emit  sendNowInitName(tr("Initializing viewer"));
	setCentralWidget(_mainViewerWidget);

	// thread-safe initialization of the OSG wrapper manager. Calling this here
	// prevents the "unsupported wrapper" messages from OSG
	osgDB::Registry::instance()->getObjectWrapperManager()->findWrapper("osg::Image");

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
	_mousePicker->setupUi(statusBar());
	_mainViewerWidget->getMainView()->addEventHandler(_mousePicker);

	_pluginManager->loadPlugins();
}

void  Atlas::initDataStructure()
{
	_mapRoot = new osg::Group;
	_mapRoot->setName("Map Root");

	_drawRoot = new osg::Group;
	_drawRoot->setName("Draw Root");
	// Draw root should not be intersected
	_drawRoot->setNodeMask(0xffffffff & (~INTERSECT_IGNORE));

	_dataRoot = new osg::Group;
	_dataRoot->setName("Data Root");

	// Init osgEarth node using the predefined .earth file
	for (int i = 0; i < MAX_SUBVIEW; i++)
	{
		QString  mode = _settingsManager->getOrAddSetting("Base mode", "projected").toString();
		QString  baseMapPath;

		if (mode == "projected")
		{
			baseMapPath = QStringLiteral("resources/earth_files/projected.earth");
		}
		else if (mode == "geocentric")
		{
			baseMapPath = QStringLiteral("resources/earth_files/geocentric.earth");
		}
		else
		{
			QMessageBox::warning(nullptr, "Warning", "Base map settings corrupted, reset to projected");
			_settingsManager->setOrAddSetting("Base mode", "projected");
			baseMapPath = QStringLiteral("resources/earth_files/projected.earth");
		}

		osg::ref_ptr<osgDB::Options>  myReadOptions = osgEarth::Registry::cloneOrCreateOptions(0);
		osgEarth::Config              c;
		c.add("elevation_smoothing", false);
		osgEarth::TerrainOptions  to(c);
		osgEarth::MapNodeOptions  defMNO;
		defMNO.setTerrainOptions(to);

		myReadOptions->setPluginStringData("osgEarth.defaultOptions", defMNO.getConfig().toJSON());

		osg::Node *baseMap = osgDB::readNodeFile(baseMapPath.toStdString(), myReadOptions);
		_mapNode[i] = osgEarth::MapNode::get(baseMap);
		_mapNode[i]->setName(QString("Map%1").arg(i).toStdString());
		_mapNode[i]->setNodeMask((SHOW_IN_WINDOW_1 << i) | SHOW_IN_NO_WINDOW);
		_mapNode[i]->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
		_mainMap[i] = _mapNode[i]->getMap();

		_mapRoot->addChild(_mapNode[i]);
	}

	_settingsManager->setGlobalSRS(_mainMap[0]->getSRS());

	// Init overlayNode with overlayerSubgraph
	// Everything in overlaySubgraph will be projected to its children
	_dataOverlay = new osgSim::OverlayNode(osgSim::OverlayNode::OBJECT_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY);
	_dataOverlay->setName("Data Overlay");
	_dataOverlay->getOrCreateStateSet()->setAttributeAndModes(
		new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
	_dataOverlay->setOverlayBaseHeight(-1);
	_dataOverlay->setOverlayTextureSizeHint(2048);
	_dataOverlay->setOverlayTextureUnit(3);

	_overlaySubgraph = new osg::Group;
	_dataOverlay->setOverlaySubgraph(_overlaySubgraph);
	_dataOverlay->addChild(_dataRoot);

	_root->addChild(_dataOverlay);
	_root->addChild(_drawRoot);
	_root->addChild(_mapRoot);

	_dataManager->registerDataRoots(_root);
}

void  Atlas::resetCamera()
{
	if (_mainMap[0]->isGeocentric())
	{
		osg::ref_ptr<osgEarth::Util::EarthManipulator>  manipulator =
			dynamic_cast<osgEarth::Util::EarthManipulator *>(_mainViewerWidget->getMainView()->getCameraManipulator());

		if (!manipulator.valid())
		{
			manipulator = new osgEarth::Util::EarthManipulator;
			_mainViewerWidget->getMainView()->setCameraManipulator(manipulator);
		}
		else
		{
			manipulator->home(0);
		}

		auto  settings = manipulator->getSettings();
		settings->setSingleAxisRotation(true);
		settings->setMinMaxDistance(10000.0, settings->getMaxDistance());
		settings->setMaxOffset(5000.0, 5000.0);
		settings->setMinMaxPitch(-90, 90);
		settings->setTerrainAvoidanceEnabled(true);
		settings->setThrowingEnabled(false);
	}
	else
	{
		MapController *manipulator = dynamic_cast<MapController *>(_mainViewerWidget->getMainView()->getCameraManipulator());

		if (!manipulator)
		{
			// Init a manipulator if not inited yet
			manipulator = new MapController(_dataRoot, _mapRoot, _mainMap[0]->getSRS());
			manipulator->setAutoComputeHomePosition(false);

			if (_settingsManager->getOrAddSetting("Camera indicator", false).toBool())
			{
				manipulator->setCenterIndicator(_mainViewerWidget->createCameraIndicator());
			}

			// Nearfar mode and ratio affect scene clipping
			auto  camera = _mainViewerWidget->getMainView()->getCamera();
			camera->setComputeNearFarMode(osg::Camera::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);

			connect(_dataManager, &DataManager::moveToNode,
			        manipulator, &MapController::fitViewOnNode);
			connect(_dataManager, &DataManager::moveToBounding,
			        manipulator, &MapController::fitViewOnBounding);

			_mainViewerWidget->getMainView()->setCameraManipulator(manipulator);
			manipulator->registerWithView(_mainViewerWidget->getMainView(), 0);
		}

		manipulator->fitViewOnNode(_mapNode[0]);
	}
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
	osgEarth::setNotifyLevel(osg::INFO);
	osgEarth::setNotifyHandler(new LogFileHandler(_log));

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
	foreach(const QString &fileName, pluginsDir.entryList(QDir::Files))
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
