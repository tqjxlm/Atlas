#pragma once
#include <QtPlugin>
#include <DrawSurfaceLine/DrawSurfaceLine.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace osg {
	class AnimationPath;
	class Geode;
	class Geometry;
}

class ModelPath : public DrawSurfaceLine
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "ModelPath.json")
	Q_INTERFACES(PluginInterface)

public:
	ModelPath();
	~ModelPath();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected:
	virtual void onLeftButton();
	virtual void onRightButton();
	virtual void onDoubleClick();
	virtual void onMouseMove();
    void updateIntersectedLine();
    osg::ref_ptr<osg::Geometry> newLine();

	osg::ref_ptr<osg::AnimationPath> CreateAnimationPath(osg::ref_ptr<osg::Vec3Array> pointarr);
	osg::ref_ptr<osg::PositionAttitudeTransform> CreateVehicleModel(osg::ref_ptr<osg::AnimationPath> path);
	float getRunTime(osg::Vec3 res, osg::Vec3 des);
	void recordCurrent();

protected:
	osg::ref_ptr<osg::Geode> _lineNode;
	osg::ref_ptr<osg::Vec3Array> _lineVertex;
	osg::ref_ptr<osg::Vec3Array> _pathPointList;//由slicerPointList抽稀
	osg::Vec3 _tmpVec3;
	osg::ref_ptr<osg::AnimationPath> _animpath;
	osg::ref_ptr<osg::Group> _animGroup;

private:
	QAction* _action;
};
