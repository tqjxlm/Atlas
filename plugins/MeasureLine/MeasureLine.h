#pragma once
#include <QtPlugin>
#include <DrawLine/DrawLine.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MeasureLine: public DrawLine
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "MeasureLine.json")
	Q_INTERFACES(PluginInterface)

public:
	MeasureLine();

	~MeasureLine();

  virtual void  setupUi(QToolBar *toolBar, QMenu *menu) override;

  virtual void  onLeftButton();

  virtual void  onDoubleClick();

  virtual void  onMouseMove();

protected:
  double                       _totoalDistance;
  osg::ref_ptr<osgText::Text>  _tmpLabel;
};
