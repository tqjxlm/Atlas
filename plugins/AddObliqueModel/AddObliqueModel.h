#pragma once
#include <QtPlugin>
#include <EarthDataInterface/EarthDataInterface.h>

#include <QMutex>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddObliqueModel : public EarthDataInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "AddObliqueModel.json")
	Q_INTERFACES(PluginInterface)

public:
	AddObliqueModel();
	~AddObliqueModel();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected slots:
	void addObliqueModel();

protected:
	void loadObliqueModel(const QString& pathXML);
  void onLoadingDone(const QString& nodeName, osg::Node *model, const osgEarth::GeoPoint &geoOrigin);

private:
	QMutex _loadingLock;
};
