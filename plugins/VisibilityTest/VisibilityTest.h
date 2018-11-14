#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class VisibilityTest : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "VisibilityTest.json")
	Q_INTERFACES(PluginInterface)

public:
	VisibilityTest();
	~VisibilityTest();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton();
	virtual void onRightButton();
	virtual void onDoubleClick();
	virtual void onMouseMove();

protected:
	QAction* _action;

	osg::ref_ptr<osg::Geometry> _visionGeometry;//可视线
	osg::ref_ptr<osg::Vec3Array> _visionVertex;//可视线顶点数组
	osg::ref_ptr<osg::Geometry> _noVisionGeometry;//不可视线
	osg::ref_ptr<osg::Vec3Array> _noVisionVertex;//不可视线顶点数组

	int _ptCount;
	osg::Vec3 _start_point, _end_point;
	osg::Vec3 _first_intersection;

	void initVisionGeode();//初始化实线
};
