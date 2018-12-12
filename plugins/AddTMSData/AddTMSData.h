#pragma once
#include <QtPlugin>
#include <EarthDataInterface/EarthDataInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddTMSData: public EarthDataInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "AddTMSData.json")
	Q_INTERFACES(PluginInterface)

public:
	AddTMSData();
	~AddTMSData();

  virtual void  setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
	void addImage();
  void addTerrain();
};
