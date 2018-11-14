#pragma once
#include <QtPlugin>
#include <DrawPolygon/DrawPolygon.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MeasureArea : public DrawPolygon
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "MeasureArea.json")
	Q_INTERFACES(PluginInterface)

public:
	MeasureArea();
	~MeasureArea();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
	virtual void onDoubleClick();
	float calcuSpatialPolygonArea();
};
