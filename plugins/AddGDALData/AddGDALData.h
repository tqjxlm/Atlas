#pragma once
#include <QtPlugin>
#include <EarthDataInterface/EarthDataInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddGDALData: public EarthDataInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "AddGDALData.json")
	Q_INTERFACES(PluginInterface)

public:
	AddGDALData();

	~AddGDALData();

  virtual void  setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
  void          addImage();

  void          addTerrain();

  void          addFeature();
};
