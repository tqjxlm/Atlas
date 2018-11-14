#pragma once
#include <QtPlugin>
#include <DrawSurfaceLine/DrawSurfaceLine.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

#include <QVector>

namespace osg {
	class Geometry;
}

class ElevationSliceShowDlg;

class ElevationSlice : public DrawSurfaceLine
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "ElevationSlice.json")
	Q_INTERFACES(PluginInterface)

public:
	ElevationSlice();
	~ElevationSlice();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
	virtual void onLeftButton();
	virtual void onRightButton();
	virtual void onDoubleClick();
	virtual void onMouseMove();
	void sliceAnalysis();

public slots:
	void clearDraw();

protected:
	ElevationSliceShowDlg* _dialogSlice;
	QVector<osg::Geometry*> _gemList;
	osg::ref_ptr<osg::Vec3Array> _pointArr;

private:
	QAction* _action;
};
