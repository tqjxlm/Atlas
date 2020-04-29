#pragma once
#include "SetRefPlane_global.h"

#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MeasureVolumePlaneSetDlg;

namespace osgText {
	class Text;
}

class SETREFPLANE_EXPORT SetRefPlane : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "SetRefPlane.json")
	Q_INTERFACES(PluginInterface)

public:
	SetRefPlane();
	~SetRefPlane();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton() override;
	virtual void onMouseMove() override;
	virtual void onRightButton() override;
	virtual void onDoubleClick() override;

	void createBasicPolygon();
	void drawPlane();
	void resetButton();

protected:
	QAction* _action;
	bool _planeDone;
	osg::Plane _refPlane;
	osg::ref_ptr<osg::Vec3Array> _planePoints;
	osg::ref_ptr<osg::Geode> _buildPlaneNode;
	osg::ref_ptr<osgText::Text> _text;

	MeasureVolumePlaneSetDlg *_planeSettingDlg;

public:
	static osg::ref_ptr<osg::Vec3Array> _vertexBP;
};
