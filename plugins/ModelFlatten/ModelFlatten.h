#pragma once
#include <QtPlugin>
#include <DrawSurfacePolygon/DrawSurfacePolygon.h>

#include <QStringList>
#include <QMap>

namespace osg {
	class MatrixTransform;
}

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class ModelFlatten : public DrawSurfacePolygon
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "ModelFlatten.json")
	Q_INTERFACES(PluginInterface)

public:
	ModelFlatten();
	~ModelFlatten();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected:
	virtual void onLeftButton();
	virtual void onDoubleClick();

protected:
	osg::ref_ptr<osg::Vec2Array> _boundary;
	double _avgHeight;
	osg::MatrixTransform* _activatedModel;
	QMap<QString, QStringList> _tempFileList;
	bool _isFlattening;

private:
	QAction* _action;
};
