#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "DataManager_global.h"
#include "../../NameSpace.h"
#include "DataFormats.h"

#include <QObject>

#include <osg/Vec3>
#include <osg/Node>

QT_BEGIN_NAMESPACE
class QProgressBar;
class QMenu;
class QAction;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace osgSim {
	class OverlayNode;
}

namespace osgEarth {
	class MapNode;
	class ModelLayer;
	class Layer;
	class SpatialReference;
	class GeoExtent;
	class Map;
}
namespace osgText{
	class Font;
}
namespace osg {
	class Group;
	class OverlayNode;
}

class SettingsManager;
class DataRecord;
class DataTree; 

class SkyBox;

class FeatureStyleSettingDlg;
class ModelLayerManager;
class ColorVisitor;
class FontVisitor;
class IconSymbolVisitor;

class DATAMANAGER_EXPORT DataManager : public QObject
{
	Q_OBJECT

public:
	DataManager(SettingsManager* settingsManager, QObject* parent);
	~DataManager();

	void reset();

	// Settings and registration
	void registerDataRoots(osg::Group* root);

	// Node visibility management
	void setWindowMask(const QString& nodeName, int mask);
	int getMask(const QString& nodeName);

	// Getters
	const osgEarth::GeoExtent* getExtent(const QString& name);
	QList<QTreeWidgetItem*> getSelectedItems();

	QMap<QString, QVector<attrib>>& getAttributeList();
	QMap<QString, QVector<feature>>& getFreatureTables();
	QMap<QString, QStringList>& getFreatureFieldList();

public slots:
	// Data management
	void newProject();
	void recordData(osg::Node* node, const QString& name, const QString& parent, bool hidden = false);
	void recordData(osgEarth::Layer* layer, const QString& name, const QString& parent, osgEarth::GeoExtent* = NULL, bool hidden = false);
	void removeData(const QString& nodeName);
	void switchData(const QString& nodeName, bool checked);

	// Style management
	//void colorChangeSlot(PrimType type, osg::Vec4 color);
	//void fontChangeSlot(osgText::Font* font, int size);
	//void sizeChangeSlot(PrimType type, int size);
	//osgEarth::ModelLayer* changeLayerStyle(
	//	std::string path, const QString& gemtype, FileType addType, std::string iconPath, float layerHeight);
	//void changeLayerStyleSlot();

	// Data manipulation
	void showDataTreeContextMenu(const QPoint &pos);
	void doubleClickTreeSlot(QTreeWidgetItem* item, int column);

	// Node attribute
	void updateAttributeList(const QString& name, const QVector<attrib>& attribs);
	void updateFeatureTable(const QString& name, const QVector<feature>& features);
	void updateFeatureFieldList(const QString& name, const QStringList& featureList);
	void showMetaDataSlot();
	//void showAttributeTableSlot();

signals:
	void moveToNode(const osg::Node*, double);
	void moveToBounding(const osg::BoundingSphere*, double);
	void stopRendering();
	void startRendering();
	void loadingProgress(int);
	void loadingDone();
	void requestContextMenu(QMenu*, QTreeWidgetItem*);
	void resetCamera();
	void showDataAttributes(const QString&);

private:
	void setupUi();
	void initDataTree();

private:
	// Program structure reference
	SettingsManager* _settingsManager;

	// Tree structures
	DataTree* _nodeTree;
	QMenu* _treeWidgetMenu;

	// Actions
	QAction* showAllChildrenAction;
	QAction* hideAllChildrenAction;
	QAction* saveNodeAction;
	QAction* deleteNodeAction;
	//QAction* rotateModelAction;
	//QAction* moveModelAction;
	//QAction* showMetatDataAction;
	//QAction* showNodeLabelAction;
	//QAction* changeLayerStyleAction;
	//QAction* showAttributeTableAction;

	// Data base
	QMap<QString, QVector<attrib>> _attributeLists;
	QMap<QString, QVector<feature>> _featureTables;
	QMap<QString, QStringList> _featureFieldList;

	// Data loading
	//ModelLayerManager* _modellyermanager;
	char* _wktInfo;
	int _countLoadingData;

	// Style visitors
	//ColorVisitor* _colorvisitor;
	//FontVisitor* _fontvisitor;
	//IconSymbolVisitor* _iconsymbolvisitor;
	//FeatureStyleSettingDlg *_featureStyleDlg;
};

#endif
