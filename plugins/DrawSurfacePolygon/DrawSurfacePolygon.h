#pragma once
#include "DrawSurfacePolygon_global.h"
#include <QtPlugin>
#include <DrawSurfaceLine/DrawSurfaceLine.h>

#include <QVector>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace osg {
  class StateSet;
  class Geometry;
}

class DRAWSURFACEPOLYGON_EXPORT DrawSurfacePolygon : public DrawSurfaceLine
{
  Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "DrawSurfacePolygon.json")
    Q_INTERFACES(PluginInterface)

public:
  DrawSurfacePolygon();
  ~DrawSurfacePolygon();
  virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected:
  virtual void onLeftButton();
  virtual void onRightButton();
  virtual void onDoubleClick();
  virtual void onMouseMove();
  virtual void drawOverlay();

  virtual osg::ref_ptr<osg::Geometry> tesselatedPolygon(osg::Vec3Array* polygon);

protected:
  static QMap<int, osg::PositionAttitudeTransform*> _polygs;
  static int _numSubDraw;

  osg::ref_ptr<osg::StateSet> _state;
  osg::ref_ptr<osg::Geometry> _lastLine;
  osg::ref_ptr<osg::Vec3Array> _contourPoints;
  osg::ref_ptr<osg::PositionAttitudeTransform> _drawnOverlay;

  osg::Vec3 _center;
  QMap<int, osg::Vec3> _centerPoints;

  double _Xmin, _Xmax, _Ymin, _Ymax;
  double _overlayAlpha;
  const float SUBGRAPH_HEIGHT = 10000.0f;

  QVector<osgEarth::GeoPoint> _contour;

private:
  QAction* _action;
};
