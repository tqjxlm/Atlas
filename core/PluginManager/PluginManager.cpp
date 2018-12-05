#include "PluginManager.h"

#include <QDir>
#include <QPluginLoader>
#include <QJsonValue>
#include <QJsonArray>
#include <QApplication>
#include <QDebug>

#include <osgViewer/View>

#include <ViewerWidget/ViewerWidget.h>
#include <PluginInterface/PluginInterface.h>
#include <DataManager/DataManager.h>
#include <MapController/MapController.h>

struct PluginEntry
{
  QString        name;
  unsigned       dependsToResolve = 0;
  QString        path;
  QSet<QString>  children;
};

PluginManager::PluginManager(QObject *parent, DataManager *dataManager, ViewerWidget *viewer):
  QObject(parent),
  _dataManager(dataManager),
  _viewerWidget(viewer)
{
  qRegisterMetaType<osgEarth::Viewpoint>("osgEarth::Viewpoint");
}

PluginManager::~PluginManager()
{
}

void  PluginManager::registerPlugin(PluginInterface *plugin)
{
	_viewerWidget->getMainView()->addEventHandler(plugin);

  connect(plugin, SIGNAL(recordData(osg::Node *,QString,QString,bool)),
          _dataManager, SLOT(recordData(osg::Node *,QString,QString,bool)));

	connect(plugin, SIGNAL(recordData(osgEarth::Layer*, QString, QString, osgEarth::GeoExtent*, bool)),
		_dataManager, SLOT(recordData(osgEarth::Layer*, QString, QString, osgEarth::GeoExtent*, bool)));

	connect(plugin, &PluginInterface::removeData, _dataManager, &DataManager::removeData);
  connect(plugin, &PluginInterface::switchData, _dataManager, &DataManager::switchData);

	connect(plugin, &PluginInterface::loadingProgress, _dataManager, &DataManager::loadingProgress);
	connect(plugin, &PluginInterface::loadingDone, _dataManager, &DataManager::loadingDone);

    osg::ref_ptr<MapController> controller = 
        dynamic_cast<MapController*>(_viewerWidget->getMainView()->getCameraManipulator());
    if (controller.valid())
        connect(plugin, &PluginInterface::setViewPoint, controller, &MapController::setViewPoint);

	_loadedPlugins.push_back(plugin);
}

void  PluginManager::loadPlugins()
{
  QDir  pluginsDir(qApp->applicationDirPath());

#if defined (Q_OS_MAC)

  if (pluginsDir.dirName() == "MacOS")
  {
		pluginsDir.cdUp();
		pluginsDir.cdUp();
		pluginsDir.cdUp();
	}

#endif
	pluginsDir.cd("plugins");

	// Parsing plugin dependencies
  foreach(const QString& fileName, pluginsDir.entryList(QDir::Files))
  {
    if ((fileName.split('.').back() == "so") || (fileName.split('.').back() == "dll"))
    {
      parseDependency(fileName, pluginsDir);
    }
	}

	// Load plugins based on denpendency tree
	while (!_readyToLoad.isEmpty())
	{
    PluginEntry *pluginEntry = _readyToLoad.front();
		_readyToLoad.pop_front();
		loadPlugin(pluginEntry);
	}

	// Notify the user about the remaining plugins
  for (auto failedPlugin : _pluginEntries)
	{
		qInfo() << "Plugin dependency cannot be resolved: " << failedPlugin->name;
	}
}

PluginInterface * PluginManager::instantiate(QObject *instance)
{
  PluginInterface *plugin = qobject_cast<PluginInterface *>(instance);

	if (plugin)
	{
		if (_pluginGroups.contains(plugin->getPluginGroup()))
		{
			registerPlugin(plugin);
			plugin->init();

      auto &pluginGroup = _pluginGroups[plugin->getPluginGroup()];
			plugin->setupUi(pluginGroup.toolBar, pluginGroup.menu);
			qInfo() << "Plugin loaded: " << plugin->getPluginName();

			return plugin;
		}
		else
		{
			qWarning() << "Plugin group not defined: " << plugin->getPluginName();
		}
	}

	return nullptr;
}

PluginEntry * PluginManager::getOrCreatePluginEntry(const QString& pluginName)
{
  PluginEntry *pluginEntry;

	if (!_pluginEntries.contains(pluginName))
	{
    pluginEntry                = new PluginEntry;
    pluginEntry->name          = pluginName;
		_pluginEntries[pluginName] = pluginEntry;
	}
	else
  {
    pluginEntry = _pluginEntries[pluginName];
  }

	return pluginEntry;
}

void  PluginManager::parseDependency(const QString& fileName, const QDir &pluginsDir)
{
  try
  {
		// Get plugin info
    QString        path = pluginsDir.absoluteFilePath(fileName);
    QPluginLoader  pluginLoader(path);
    bool           debug      = pluginLoader.metaData()["debug"].toBool();
    QJsonObject    metaData   = pluginLoader.metaData()["MetaData"].toObject();
    QString        pluginName = metaData["Name"].toString();

		if (debug)
    {
      pluginName += 'd';
    }

		// Get or create a record
    PluginEntry *pluginEntry = getOrCreatePluginEntry(pluginName);
		pluginEntry->path = path;

		// Resolve dependencies
    QJsonValue  deps = metaData["Dependencies"];

		if (deps.isArray() && !deps.toArray().isEmpty())
		{
      for (QJsonValue plugin : deps.toArray())
			{
				// Register info for parent
        QString  dependName = plugin.toObject()["Name"].toString();

				if (debug)
        {
          dependName += 'd';
        }

				pluginEntry->dependsToResolve++;
        PluginEntry *parent = getOrCreatePluginEntry(dependName);
				parent->children.insert(pluginName);
			}
		}
		else
		{
			_readyToLoad.push_back(pluginEntry);
		}
	}
  catch (...)
  {
		qWarning() << "Plugin meta not valid: " << fileName;
	}
}

void  PluginManager::loadPlugin(PluginEntry *pluginEntry)
{
  emit  sendNowInitName(tr("Init plugin: ") + pluginEntry->name);

	// Mark the plugin as parsed
	_pluginEntries.remove(pluginEntry->name);

	// Try load plugin
  QPluginLoader  pluginLoader(pluginEntry->path);
  QObject       *instance = pluginLoader.instance();

	if (instance)
	{
		// Try init plugin
    PluginInterface *plugin = instantiate(instance);

		if (!plugin)
    {
      return;
    }

		// Resolve related dependencies
    for (auto childName : pluginEntry->children)
		{
      auto  childPlugin = _pluginEntries[childName];
			childPlugin->dependsToResolve--;

			if (childPlugin->dependsToResolve == 0)
			{
				_readyToLoad.push_back(childPlugin);
			}
		}
	}
	else
	{
		qWarning() << "Plugin loading failed: [" << pluginEntry->path
               << "] " << pluginLoader.errorString();
	}
}

void  PluginManager::loadContextMenu(QMenu *contextMenu, QTreeWidgetItem *selectedItem)
{
  for (auto plugin : _loadedPlugins)
	{
		plugin->loadContextMenu(contextMenu, selectedItem);
	}
}

void  PluginManager::registerPluginGroup(const QString& name, QToolBar *toolBar, QMenu *menu)
{
	_pluginGroups[name] = { name, toolBar, menu };
}
