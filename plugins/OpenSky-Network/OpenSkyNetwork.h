#ifndef OPENSKYNETWORK_H
#define OPENSKYNETWORK_H

#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>
#include "openskyfetcher.h"

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class OpenSkyNetwork: public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "OpenSkyNetwork.json")
	Q_INTERFACES(PluginInterface)

public:
	OpenSkyNetwork();

	~OpenSkyNetwork();

  virtual void  setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
  void          toggled(bool checked);

private:
  QAction        *_action;
  OpenSkyFetcher *mOpenSkyFetcher;
};

#endif // OPENSKYNETWORK_H
