#include "PluginManager.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonValue>
#include <QPluginLoader>
#include <QSysInfo>

#include <osgViewer/View>

#include <DataManager/DataManager.h>
#include <MapController/MapController.h>
#include <PluginInterface/PluginInterface.h>
#include <ViewerWidget/ViewerWidget.h>

struct PluginEntry
{
    QString name;
    unsigned unresolvedDependency = 0;
    QString path;
    QSet<QString> children;
};

PluginManager::PluginManager(QObject* parent, DataManager* dataManager,
                             ViewerWidget* viewer) : QObject(parent),
    _dataManager(dataManager),
    _viewerWidget(viewer)
{
    qRegisterMetaType<osgEarth::Viewpoint>("osgEarth::Viewpoint");
}

PluginManager::~PluginManager()
{
}

void PluginManager::registerPlugin(PluginInterface* plugin)
{
    _viewerWidget->getMainView()->addEventHandler(plugin);

    connect(plugin, SIGNAL(recordData(osg::Node*,QString,QString,bool)),
            _dataManager, SLOT(recordData(osg::Node*,QString,QString,bool)));

    connect(plugin,
            SIGNAL(recordData(osgEarth::Layer*,QString,QString,osgEarth::GeoExtent*,bool)),
            _dataManager,
            SLOT(recordData(osgEarth::Layer*,QString,QString,osgEarth::GeoExtent*,bool)));

    connect(plugin, &PluginInterface::removeData, _dataManager, &DataManager::removeData);
    connect(plugin, &PluginInterface::switchData, _dataManager, &DataManager::switchData);

    connect(plugin, &PluginInterface::loadingProgress, _dataManager, &DataManager::loadingProgress);
    connect(plugin, &PluginInterface::loadingDone, _dataManager, &DataManager::loadingDone);

    osg::ref_ptr<MapController> controller =
        dynamic_cast<MapController*>(_viewerWidget->getMainView()->getCameraManipulator());

    if (controller.valid())
    {
        connect(plugin, &PluginInterface::setViewPoint, controller, &MapController::setViewPoint);
    }

    _loadedPlugins.push_back(plugin);
}

void PluginManager::loadPlugins()
{
    QDir pluginsDir(qApp->applicationDirPath());

    pluginsDir.cd("plugins");

    // Build a dependency tree
    foreach (const QString &fileName, pluginsDir.entryList(QDir::Files))
    {
        QString extension = fileName.split('.').back();

        if ((extension == "so") || (extension == "dll") || (extension == "dylib"))
        {
            parseDependency(fileName, pluginsDir);
        }
    }

    // Load plugins with free nodes of the dependency tree
    while (!_resolvedPlugins.isEmpty())
    {
        PluginEntry* pluginEntry = _resolvedPlugins.front();
        _resolvedPlugins.pop_front();
        loadPlugin(pluginEntry);
    }

    // Notify the user about the remaining plugins
    for (auto failedPlugin : _unresolvedPlugins)
    {
        qWarning() << "Plugin dependency cannot be resolved: " << failedPlugin->name;
    }
}

PluginInterface* PluginManager::instantiatePlugin(QObject* instance)
{
    PluginInterface* plugin = qobject_cast<PluginInterface*>(instance);

    if (plugin)
    {
		QString group = plugin->getPluginGroup();
        if (_pluginGroups.contains(group))
        {
            registerPlugin(plugin);
            plugin->init();

            auto &pluginGroup = _pluginGroups[group];
            plugin->setupUi(pluginGroup.toolBar, pluginGroup.menu);
            qInfo() << "Plugin loaded: " << group << ":" << plugin->getPluginName();

            return plugin;
        }
        else
        {
            qWarning() << "Plugin group not defined: " << plugin->getPluginName();
        }
    }

    return nullptr;
}

PluginEntry* PluginManager::getOrCreatePluginEntry(const QString &pluginName)
{
    PluginEntry* pluginEntry;

    if (!_unresolvedPlugins.contains(pluginName))
    {
        pluginEntry = new PluginEntry;
        pluginEntry->name = pluginName;
        _unresolvedPlugins[pluginName] = pluginEntry;
    }
    else
    {
        pluginEntry = _unresolvedPlugins[pluginName];
    }

    return pluginEntry;
}

void PluginManager::parseDependency(const QString &fileName, const QDir &pluginsDir)
{
    try
    {
        // Get plugin info
        QString path = pluginsDir.absoluteFilePath(fileName);
        QPluginLoader pluginLoader(path);
        QJsonObject metaData = pluginLoader.metaData()["MetaData"].toObject();
        QString pluginName = metaData["Name"].toString();
        bool debug = pluginLoader.metaData()["debug"].toBool();
        bool macos = QSysInfo::kernelType() == "darwin";

        // Get or create a record
        PluginEntry* pluginEntry = getOrCreatePluginEntry(pluginName);
        pluginEntry->path = path;

        // Resolve dependencies
        QJsonValue deps = metaData["Dependencies"];

        if (deps.isArray() && !deps.toArray().isEmpty())
        {
            for (QJsonValue dependency : deps.toArray())
            {
                pluginEntry->unresolvedDependency++;

                // Register this plugin a parent node
                QString dependName = dependency.toObject()["Name"].toString();

                PluginEntry* parent = getOrCreatePluginEntry(dependName);
                parent->children.insert(pluginName);
            }
        }
        else
        {
            _resolvedPlugins.push_back(pluginEntry);
        }
    }
    catch (...)
    {
        qWarning() << "Plugin meta not valid: " << fileName;
    }
}

void PluginManager::loadPlugin(PluginEntry* pluginEntry)
{
    emit sendNowInitName(tr("Init plugin: ") + pluginEntry->name);

    // Mark the plugin as parsed
    _unresolvedPlugins.remove(pluginEntry->name);

    // Try load plugin
    QPluginLoader pluginLoader(pluginEntry->path);
    QObject* instance = pluginLoader.instance();

    if (instance)
    {
        // Try init plugin
        PluginInterface* plugin = instantiatePlugin(instance);

        if (plugin)
        {
            // Resolve related dependencies
            for (auto childName : pluginEntry->children)
            {
                auto childPlugin = _unresolvedPlugins[childName];
                childPlugin->unresolvedDependency--;

                if (childPlugin->unresolvedDependency == 0)
                {
                    _resolvedPlugins.push_back(childPlugin);
                }
            }

            return;
        }
    }

    qWarning() << "Plugin loading failed: [" << pluginEntry->path
               << "] " << pluginLoader.errorString();
}

void PluginManager::loadContextMenu(QMenu* contextMenu, QTreeWidgetItem* selectedItem)
{
    for (auto plugin : _loadedPlugins)
    {
        plugin->loadContextMenu(contextMenu, selectedItem);
    }
}

void PluginManager::registerPluginGroup(const QString &name, QPair<QToolBar*, QMenu*> entry)
{
    _pluginGroups[name] = {name, entry.first, entry.second};
}
