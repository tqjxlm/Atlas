#pragma once

#include <QObject>
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class DrawCircle: public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "DrawCircle.json")
	Q_INTERFACES(PluginInterface)

public:
	DrawCircle();

	~DrawCircle();

  virtual void                 setupUi(QToolBar *toolBar, QMenu *menu) override;

  virtual void                 onLeftButton();

  virtual void                 onMouseMove();

  virtual void                 onRightButton();

  virtual void                 onDoubleClick();

protected:
  void                         initCircle(float Radius);

  osg::ref_ptr<osg::Geometry>  createCircle(float Radius, osg::Vec3 pos);

protected:
  osg::Vec3  _center;
  osg::Vec3  _end;

  float    _circle[72][2];
  QAction *_action;
};
