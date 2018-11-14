#pragma once
#include "DrawSurfaceLine_global.h"

#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

#include <osgSim/ElevationSlice>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace osg {
	class Geometry;
}

namespace osgSim {
	class ElevationSlice;
}

namespace osgUtil {
	class IntersectionVisitor;
}

// A drawing tool to draw lines that are projected to the surface of _dataRoot
class DRAWSURFACELINE_EXPORT DrawSurfaceLine : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "DrawSurfaceLine.json")
	Q_INTERFACES(PluginInterface)

public:
	DrawSurfaceLine();
	~DrawSurfaceLine();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected:
    virtual void onLeftButton() override;
	virtual void onRightButton() override;
	virtual void onDoubleClick() override;
	virtual void onMouseMove() override;

    // A new intersected line
	osg::ref_ptr<osg::Geometry> newLine();

    // Update the intersected line with new starting and ending points
    void updateIntersectedLine();

    // Smoothing and filter out abnormal intersections
    osg::ref_ptr<osg::Vec3Array> lineSmoothing(osg::ref_ptr<osg::Vec3Array> points);

protected:
	osg::Vec3 _startPoint;
	osg::Vec3 _lastPoint;
	osg::Vec3 _endPoint;
	osgSim::ElevationSlice _slicer;
	osg::ref_ptr<osgUtil::IntersectionVisitor> _intersectionVisitor;
    osg::ref_ptr<osg::PositionAttitudeTransform> _zOffsetNode;
    osg::ref_ptr<osg::Vec3Array> _slicerPointList;
    osg::ref_ptr<osg::Geometry> _lineGeom;
    double _zOffset = 0.5;

private:
	QAction* _action;
};
