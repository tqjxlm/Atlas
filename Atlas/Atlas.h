#ifndef ATLAS_H
#define ATLAS_H

#include <AtlasMainWindow/AtlasMainWindow.h>

#include "../NameSpace.h"

#include <osg/Object>
#include <osg/BoundingSphere>

class DataManager;
class SettingsManager;
class ViewerWidget;
class PluginManager;
class MousePicker;

namespace osg {
	class Group;
	class PositionAttitudeTransform;
}

namespace osgSim {
	class OverlayNode;
}

namespace osgText {
	class Font;
}

namespace osgEarth {
	class Map;
	class MapNode;
}

class Atlas : public AtlasMainWindow
{
	Q_OBJECT

public:
	Atlas(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
	~Atlas();

	void initAll();

private:
	void initCore();
	void initDataStructure();
	void initPlugins();
	void initLog();

    void setupUi();
    void collectInitInfo();

public slots:
	void about();

	// View related slots
	void resetCamera();

signals:
	// For splash screen
	void sendTotalInitSteps(int);
	void sendNowInitName(QString);

private:
	// Core components
	std::ofstream* _log;

	// Managers
	DataManager* _dataManager;
	SettingsManager* _settingsManager;
	ViewerWidget* _mainViewerWidget;
	PluginManager* _pluginManager;
	MousePicker* _mousePicker;

	// OSG main roots and nodes
	osg::ref_ptr<osg::Group> _root;
	osg::ref_ptr<osgSim::OverlayNode> _overlayNode;
	osg::ref_ptr<osg::PositionAttitudeTransform> _overlaySubgraph;
	osg::ref_ptr<osg::PositionAttitudeTransform> _drawRoot;
	osg::ref_ptr<osg::PositionAttitudeTransform> _dataRoot;

	// OSGEarth map roots and nodes
	osg::ref_ptr<osgEarth::MapNode> _mapNode[4];
	osg::ref_ptr<osgEarth::Map> _mainMap[4];
};

#endif 