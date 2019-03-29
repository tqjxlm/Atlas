#include "OpenSkyNetwork.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <ViewerWidget/ViewerWidget.h>
#include <qmessagebox.h>
#include "dataptree.h"

OpenSkyNetwork::OpenSkyNetwork()
{
  _pluginCategory = "Draw";
  _pluginName     = "OpenSkyNetwork";
  mOpenSkyFetcher = new OpenSkyFetcher(_mapNode[0], _mainViewer->getMainView());
}

OpenSkyNetwork::~OpenSkyNetwork()
{
}

void  OpenSkyNetwork::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
  _action->setObjectName(QStringLiteral("OpenSkyNetworkAction"));
  QIcon  icon19;
  icon19.addFile(QStringLiteral("resources/OpenSkyNetwork/opensky-network.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon19);
  _action->setCheckable(true);
  _action->setText(tr("OpenSkyNetwork"));
  _action->setToolTip(tr("Enable OpenSky-network"));

	toolBar->addAction(_action);
	menu->addAction(_action);

  connect(_action, &QAction::toggled, this, &OpenSkyNetwork::toggled);
}

void  OpenSkyNetwork::toggled(bool checked)
{
  if (checked)
  {
    mOpenSkyFetcher->run();

    _action->setText("Disable");
  }
  else
  {
    mOpenSkyFetcher->stopTimer();
    mOpenSkyFetcher->quit();
    _action->setText("Enable");
  }
}
