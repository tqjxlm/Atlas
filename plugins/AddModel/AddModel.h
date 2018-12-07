#pragma once
#include <QtPlugin>
#include <EarthDataInterface/EarthDataInterface.h>

namespace osg {
	class Node;
	class PositionAttitudeTransform;
}

namespace osgEarth {
  namespace Annotation {
    class ModelNode;
  }
}

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddModel : public EarthDataInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "AddModel.json")
  Q_INTERFACES(PluginInterface)

public:
	AddModel();
	~AddModel();

	virtual void onLeftButton();
	virtual void onRightButton();
	virtual void onMouseMove();
	virtual void finish();
	virtual bool loadModel(std::string filePath);
	virtual void setupUi(QToolBar* toolBar, QMenu* menu);

public slots:
	virtual void toggle(bool checked = true) override;

protected:
  osg::ref_ptr<osg::PositionAttitudeTransform> _modelNode;
	QString _filepath;

	QAction* _modelAction1;
	QAction* _modelAction2;
	QAction* _modelAction3;
	QAction* _modelFromFileAction;
};

