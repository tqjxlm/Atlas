#include "DrawCircle.h"

#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QString>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

static const double  PI = osg::PI;

DrawCircle::DrawCircle()
{
  _pluginName     = tr("Circle");
  _pluginCategory = "Draw";

  QMap<QString, QVariant>  customStyle;
  customStyle["Fill color"] = QColor(174, 209, 234);

  getOrAddPluginSettings("Draw style", customStyle);
}

DrawCircle::~DrawCircle(void)
{
}

void  DrawCircle::setupUi(QToolBar *toolBar, QMenu *menu)
{
  _action = new QAction(_mainWindow);
  _action->setObjectName(QStringLiteral("drawCircleAction"));
  _action->setCheckable(true);
  QIcon  icon11;
  icon11.addFile(QStringLiteral("resources/icons/drawcircle.png"), QSize(), QIcon::Normal, QIcon::Off);
  _action->setIcon(icon11);
  _action->setText(tr("Circle"));
  _action->setToolTip(tr("Draw Circle"));

  connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
  registerMutexAction(_action);

  toolBar->addAction(_action);
  menu->addAction(_action);
}

void  DrawCircle::onLeftButton()
{
  if (!_isDrawing)
  {
    beginDrawing();

    _currentDrawNode = new osg::Geode;
    _currentAnchor->addChild(_currentDrawNode);
    _currentDrawNode->addDrawable(createPointGeode(_anchoredWorldPos, _intersections.begin()->getLocalIntersectNormal()));

    _center = _anchoredWorldPos;
  }
}

void  DrawCircle::onRightButton()
{
  if (_isDrawing)
  {
    _currentAnchor->removeChild(_currentDrawNode);
    endDrawing();
  }
}

void  DrawCircle::onMouseMove()
{
  if (_isDrawing)
  {
    _end = _anchoredWorldPos;
    float  radiance = sqrt(
      (_end.x() - _center.x()) * (_end.x() - _center.x())
      + (_end.y() - _center.y()) * (_end.y() - _center.y())
      + (_end.z() - _center.z()) * (_end.z() - _center.z()));

    _currentDrawNode->removeDrawables(1);
    _currentDrawNode->addDrawable(createCircle(radiance, _center));
  }
}

void  DrawCircle::onDoubleClick()
{
  if (_isDrawing)
  {
    endDrawing();
    recordCurrent();
  }
}

osg::ref_ptr<osg::Geometry>  DrawCircle::createCircle(float Radius, osg::Vec3 pos)
{
  osg::ref_ptr<osg::Geometry>  geom = new osg::Geometry();

  initCircle(Radius);

  osg::ref_ptr<osg::Vec3Array>  v = new osg::Vec3Array();

  for (int i = 0; i < 72; i++)
  {
    v->push_back(osg::Vec3(pos.x() + _circle[i][0], pos.y() + _circle[i][1], pos.z() + 0.5));
  }

  geom->setVertexArray(v.get());

  osg::ref_ptr<osg::Vec4Array>  colors = new osg::Vec4Array;
  colors->push_back(_style.fillColor);
  geom->setColorArray(colors, osg::Array::BIND_OVERALL);

  geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, 72));

  return geom.get();
}

void  DrawCircle::initCircle(float Radius)
{
  float  angle = 0;

  for (int i = 0; i < 72; i++)
  {
    _circle[i][0] = Radius * cos(angle);
    _circle[i][1] = Radius * sin(angle);

    angle += 2.0 * PI / 72.0f;
  }
}
