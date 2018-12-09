#ifndef DATATREE_H
#define DATATREE_H

#include <QTreeWidget>

#include "DataRecord.h"

namespace osg {
	class Node;
}

namespace osgSim {
    class Group;
    class OverlayNode;
}

namespace osgEarth {
	class Layer;
	class Map;
	class Layer;
}

class DataManager;
class SettingsManager;

typedef QMap<QString, DataRecord*> RecordDict;

class DataTree : public QTreeWidget
{
	Q_OBJECT

public:
	DataTree(SettingsManager* settings, QWidget *parent);
	~DataTree();

	void clear();

	void addRecord(osg::Node* node, const QString& name, const QString& parentName, bool hidden = false);
	void addRecord(osgEarth::Layer* layer, const QString& name, const QString& parentName, osgEarth::GeoExtent* extent = NULL, bool hidden = false);
	void removeRecord(const QString& name);

	bool saveNode(const QString& nodeName);
	bool saveNodes(const QStringList & nodeNames);

	void setWindowMask(const QString& name, int mask);

	DataRecord* getRecord(const QString& name);
	osg::Node* getNode(const QString& name);
	osgEarth::Layer* getLayer(const QString& name);

public slots:
	void deleteNodeSlot();
	void saveNodeSlot();
	void switchDataSlot(QTreeWidgetItem* item, int column);
	void switchAllSlot(int enabled);

private:
	// Generate a name from expeted name to avoid collision
	QString resolveName(const QString& expectedName);

	// Remove record from data tree and render tree
	void removeRecord(DataRecord* record);

	// Actually remove data from render tree
	void removeDataRendering(DataRecord* record);

    // Remove osg node
    void removeNode(osg::Node* node);

    // Switch a data record
    void switchRecord(const QString& nodeName, bool checked);

    // Switch osg node
    void switchNode(osg::Node* node, bool checked);

	DataRecord* getParent(const QString& parentName);
	void switchAll(DataRecord *item, bool isVisible);

private:
	RecordDict _dataRecords;
	DataRecord* _rootTreeItem;
	osgEarth::Map* _mainMap[MAX_SUBVIEW];
    osgSim::OverlayNode* _overlayNode;
    SettingsManager* _settingsManager;

	friend DataManager;
};

#endif // DATATREE_H
