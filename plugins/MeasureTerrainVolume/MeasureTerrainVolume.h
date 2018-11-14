#pragma once
#include <QtPlugin>
#include <MeasureTerrainArea/MeasureTerrainArea.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MeasureTerrainVolume : public MeasureTerrainArea
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "MeasureTerrainVolume.json")
	Q_INTERFACES(PluginInterface)

public:
	MeasureTerrainVolume();
	~MeasureTerrainVolume();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton();
	virtual void onRightButton();
	virtual void onDoubleClick();
	virtual void onMouseMove();

	bool calculateVolume();
	float volInBoundary(osg::Node* node, osg::Vec3Array* boundary);

protected:
	bool _planeDone;
	osg::Plane _refPlane;

private:
	QAction* _action;
};
