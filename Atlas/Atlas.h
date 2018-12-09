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

namespace osg
{
class Group;
class PositionAttitudeTransform;
}

namespace osgSim
{
class OverlayNode;
}

namespace osgText
{
class Font;
}

namespace osgEarth
{
class Map;
class MapNode;
}

class Atlas: public AtlasMainWindow
{
	Q_OBJECT

public:
	Atlas(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);

	~Atlas();

  void  initAll();

private:
  void  initCore();

  void  initDataStructure();

  void  initPlugins();

  void  initLog();

  void  setupUi();

  void  collectInitInfo();

public slots:
  void  about();

	// View related slots
  void  resetCamera();

signals:
	// For splash screen
  void  sendTotalInitSteps(int);

  void  sendNowInitName(const QString&);

private:
  std::ofstream *_log;

  DataManager     *_dataManager;
  SettingsManager *_settingsManager;
  PluginManager   *_pluginManager;
  osg::ref_ptr<ViewerWidget>    _mainViewerWidget;
  osg::ref_ptr<MousePicker>     _mousePicker;

	// Root for all
  osg::ref_ptr<osg::Group>                      _root;
  // Root for all data that can be projected on
  osg::ref_ptr<osgSim::OverlayNode>             _dataOverlay;
  // Node that is projected to the _dataOverlay
  osg::ref_ptr<osg::Group>  _overlaySubgraph;
  // Root for all drawings
  osg::ref_ptr<osg::Group>  _drawRoot;
  // Root for osgEarth maps
  osg::ref_ptr<osg::Group>  _mapRoot;
  // Root for osg format data
  osg::ref_ptr<osg::Group>  _dataRoot;

  osg::ref_ptr<osgEarth::MapNode>  _mapNode[MAX_SUBVIEW];
  osg::ref_ptr<osgEarth::Map>      _mainMap[MAX_SUBVIEW];
};

#endif
