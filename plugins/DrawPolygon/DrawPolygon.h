#pragma once
#include "DrawPolygon_global.h"
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class DRAWPOLYGON_EXPORT DrawPolygon : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "DrawPolygon.json")
	Q_INTERFACES(PluginInterface)

public:
	DrawPolygon();
	~DrawPolygon();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton();
	virtual void onMouseMove();
	virtual void onRightButton();
	virtual void onDoubleClick();

	virtual osg::ref_ptr<osg::Geometry> createPolygon();

protected:
	QAction* _action;

	osg::ref_ptr<osg::Vec3Array> _polyVecArray;
	osg::ref_ptr<osg::Geometry> _currentPolygon;
};
