#pragma once

#include "MousePicker_global.h"

#include "../../NameSpace.h"

#include <QObject>
#include <osgGA/GUIEventHandler>
#include <osgEarth/GeoData>

QT_BEGIN_NAMESPACE
class QStatusBar;
class QLabel;
QT_END_NAMESPACE

namespace osg {
  class Group;
}

namespace osgEarth {
	class MapNode;
	class Map;
	class SpatialReference;
}

namespace osgSim
{
class OverlayNode;
}

namespace osgViewer
{
class View;
}

class DataManager;
class SettingsManager;
class ViewerWidget;

/** MousePicker is the public parent of all interactions that happened
 * in the osg scene, eg. various plugins.
 *
 * It provides the coordinate and other information about the point under
 * the current mouse arrow, and ensures that this information is calculated
 * only once in an event frame.
 *
 * It provides interfaces for various event handlers, including mouse and
 * keyboard events.
 *
 * It also keeps records of some global nodes, and maintains the
 * connection with the main program, ie. Atlas class, so that any plugins
 * can inherit and get easy access.
 */
class MOUSEPICKER_EXPORT  MousePicker: public QObject, public osgGA::GUIEventHandler
{
	Q_OBJECT

public:
	MousePicker();

	virtual ~MousePicker();

	// Connect action with the main program as well as the associated QAction
  void          registerData(QWidget *mainWindow, DataManager *dataManager, ViewerWidget *mainViewer, osg::Group *root,
                             const osgEarth::SpatialReference *globalSRS);

  void          registerSetting(SettingsManager *settingsMenager);

	// Return true if the point has valid OSGEarth coordinate
  bool          pointValid();

	// Default operation for GUI Event Handler
  virtual void  defaultOperation(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa);

    void setupUi(QStatusBar* statusBar);

protected:
	// Public main entrance for GUIEventHandler
  virtual bool  handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa);

private:
  // Intersect with the scene and update coord to main Ui
  virtual void  pick(osgViewer::View *view, const osgGA::GUIEventAdapter &ea);

  // Intersect with the scene and update info about the intersected point
  virtual void  getPos(osgViewer::View *view, const osgGA::GUIEventAdapter &ea);

signals:
	// Signal to update mouse type
  void  changeMouseType(unsigned type);

protected:
  bool  _activated;

	// Infomation of the intersected point
  static osgEarth::GeoPoint _currentGeoPos;
  static osg::Vec3d _currentLocalPos;
  static osg::Vec3d _currentWorldPos;
  static osgUtil::LineSegmentIntersector::Intersections _intersections;
  static osgUtil::LineSegmentIntersector::Intersection _nearestIntesection;

  // Global nodes defined and initialized in main program
  static osg::ref_ptr<osg::Group>                      _root;
  static osg::ref_ptr<osg::Group>                      _mapRoot;
  static osg::ref_ptr<osg::Group>             _dataRoot;
  static osg::ref_ptr<osg::Group>  _subgraph;
  static osg::ref_ptr<osg::Group>  _drawRoot;
  static osg::ref_ptr<osgSim::OverlayNode>             _overlayNode;
  static osg::ref_ptr<osgEarth::MapNode>               _mapNode[MAX_SUBVIEW];
  static osg::ref_ptr<osgEarth::Map>                   _mainMap[MAX_SUBVIEW];

	// Access to other components of this program
  static DataManager                                    *_dataManager;
  static SettingsManager                                *_settingsManager;
  static ViewerWidget                                   *_mainViewer;
  static QWidget                                        *_mainWindow;
  static osg::ref_ptr<const osgEarth::SpatialReference>  _globalSRS;
  static const char                                     *_globalWKT;

private:
  static bool _isValid;

  static QLabel *_labelWorldCoord;
  static QLabel *_labelGeoCoord;
};
