#include "SettingsManager.h"

#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QProcess>
#include <QAction>
#include <QMenu>

SettingsManager::SettingsManager(QObject* parent):
  QObject(parent),
  _globalSettings()
{
}

SettingsManager::~SettingsManager()
{
	_globalSettings.sync();
}

void SettingsManager::setOrAddSetting(const QString& key, const QVariant & value)
{
  if (value.isValid())
  {
    _globalSettings.setValue(key, value);
    _globalSettings.sync();
  }
}

QVariant SettingsManager::getOrAddSetting(const QString& key, const QVariant & defaultValue)
{
  auto  found = _globalSettings.value(key);

  if (!found.isValid())
  {
    if (defaultValue.isValid())
    {
      _globalSettings.setValue(key, defaultValue);
      _globalSettings.sync();
    }

    return defaultValue;
  }

  return found;
}

void  SettingsManager::setupUi(QMenu *menu)
{
  // Reset settings
  QAction *resetAction = new QAction;

  resetAction->setObjectName(QStringLiteral("resetSettingsAction"));
  resetAction->setText(tr("Restore Settings"));
  resetAction->setToolTip("Restore all settings to default");
  resetAction->setStatusTip(resetAction->toolTip());
  connect(resetAction, &QAction::triggered, this, &SettingsManager::reset);

  // Map mode settings
  QAction* mapModeAction = new QAction(this);
  mapModeAction->setText(tr("Switch mode"));
  mapModeAction->setToolTip(mapModeAction->text());
  mapModeAction->setStatusTip(mapModeAction->text());
  connect(mapModeAction, &QAction::triggered, [this]() {
    QString mode = getOrAddSetting("Base mode", "projected").toString();
    if (mode == "geocentric")
      setOrAddSetting("Base mode", "projected");
    else
      setOrAddSetting("Base mode", "geocentric");
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
  });

  for (QAction *action : menu->actions())
  {
    if (action->objectName() == "actionExit")
    {
      menu->insertAction(action, mapModeAction);
      menu->insertAction(action, resetAction);
      menu->insertSeparator(action);
      break;
    }
  }
}

void  SettingsManager::setGlobalSRS(const osgEarth::SpatialReference *globalSRS)
{
	_globalSRS = globalSRS;
}

const osgEarth::SpatialReference * SettingsManager::getGlobalSRS()
{
	return _globalSRS;
}

void  SettingsManager::reset()
{
  int  result = QMessageBox::question(NULL,
                                      tr("Restoring settings"),
                                      tr("About to restore all settings.") + "\n\n" + tr("Requires restarting program, continue?"),
                                      QMessageBox::Yes, QMessageBox::No);

  if (result == QMessageBox::Yes)
  {
    _globalSettings.clear();
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
  }
}
