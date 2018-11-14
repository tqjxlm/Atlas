#pragma once
#include "MeasureTerrainArea_global.h"
#include <QtPlugin>
#include <DrawSurfacePolygon/DrawSurfacePolygon.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MEASURETERRAINAREA_EXPORT MeasureTerrainArea : public DrawSurfacePolygon
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "MeasureTerrainArea.json")
	Q_INTERFACES(PluginInterface)

public:
	MeasureTerrainArea();
	~MeasureTerrainArea();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
	virtual void onDoubleClick();
	virtual void onLeftButton();

protected:
	bool calculateArea();
	void showTxtAtCenter(std::string& txt);
	void setBoundingPolytope(osg::Polytope& boundingPolytope, const osg::Vec3Array* ctrlPoints, const osg::Vec3& up);
	float areaInBoundary(osg::Node* node, osg::Vec3Array* boundary);

protected:
	osg::ref_ptr<osg::Vec3Array> _contour;

private:
	QAction* _action;
};
