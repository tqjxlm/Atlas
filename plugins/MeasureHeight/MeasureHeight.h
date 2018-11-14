#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace osgText {
	class Text;
}

namespace osg {
	class Geometry;
}

class MeasureHeight : public PluginInterface
{
	Q_OBJECT
		Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "MeasureHeight.json")
		Q_INTERFACES(PluginInterface)

public:
	MeasureHeight();
	~MeasureHeight();
	virtual void setupUi(QToolBar *toolBar, QMenu* menu) override;

protected:
	virtual void onLeftButton();
	virtual void onRightButton();
	virtual void onMouseMove();
	void updateLines();
	void updateText();

protected:
	osg::ref_ptr<osg::Geometry> _hLineUp;
	osg::ref_ptr<osg::Geometry> _hLineLow;
	osg::ref_ptr<osg::Geometry> _vLine;
	osg::ref_ptr<osgText::Text> _text;

	osg::Vec3 _startPoint;
	osg::Vec3 _highPoint;
	osg::Vec3 _lowPoint;
	osg::Vec3 _hDir;
	osg::Vec3 _vDir;
	double _height;

private:
	QAction* _action;
};
