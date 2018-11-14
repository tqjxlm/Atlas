#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

namespace osg {
	class Node;
	class PositionAttitudeTransform;
}

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddModel : public PluginInterface
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
	virtual void showNewModel();
	virtual void setupUi(QToolBar* toolBar, QMenu* menu);

public slots:
	void addModelFromDB(QString modelName,QString modelFilePath,osg::Vec3 pos,osg::Vec3 norl);
	virtual void toggle(bool checked = true) override;

protected:
	virtual void recordCurrent();

protected:
	osg::ref_ptr<osg::Node> _modelFile;
	QString _filepath;
	osg::ref_ptr<osg::PositionAttitudeTransform> _pat;
	QString _modelUniqID;
	QString _modelName;

	QAction* _modelAction1;
	QAction* _modelAction2;
	QAction* _modelAction3;
	QAction* _modelFromFileAction;
};

