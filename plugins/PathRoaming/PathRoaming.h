#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MapController;

namespace osg {
	class AnimationPath;
}

class PathRoaming : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "PathRoaming.json")
	Q_INTERFACES(PluginInterface)

public:
	PathRoaming();
	~PathRoaming();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton();
	virtual void onMouseMove();
	virtual void onRightButton();
	virtual void onDoubleClick();
	osg::Geode* createBox(osg::Vec3 center);
	float getRunTime(osg::Vec3 res, osg::Vec3 des);
	osg::AnimationPath* createPath();

public slots:
	void roamRecordSlot(bool checked);
	void roamPlaybackSlot();

private:
	osg::ref_ptr<osg::Geometry> _pathGeometry;
	osg::ref_ptr<osg::Vec3Array> _pathVertex;
	osg::ref_ptr<osg::Vec3Array> _points;
	osg::ref_ptr<osg::Geode> _pathNode;
	osg::ref_ptr<osg::Group> _allPathNode;
	int _DCCount;
	osg::Matrixd _cameraPos;

	osg::ref_ptr<MapController> _manipulator;

	QAction* _action;
	QAction* roamRecordAction;
	QAction* roamPlaybackAction;
};
