#pragma once
#include <QtPlugin>
#include <EarthDataInterface/EarthDataInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddXYZData : public EarthDataInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "AddXYZData.json")
	Q_INTERFACES(PluginInterface)

public:
	AddXYZData();
	~AddXYZData();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
	void addImage();
  void addTerrain();
};
