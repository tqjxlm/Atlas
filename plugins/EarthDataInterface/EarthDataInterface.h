#pragma once

#include "EarthDataInterface_global.h"

#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

#include <DataManager/DataFormats.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
class QToolButton;
QT_END_NAMESPACE

namespace osgEarth {
	class ModelLayer;
	class Layer;
	class GeoExtent;

	namespace Symbology {
		class Style;
	}
}

class ModelLayerManager;

class EARTHDATAINTERFACE_EXPORT EarthDataInterface : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "EarthDataInterface.json")
	Q_INTERFACES(PluginInterface)

public:
	EarthDataInterface();
	~EarthDataInterface();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
	virtual void init() override;

public slots:
	void showDataAttributes(QString nodeName);

protected:
	enum DataType {
		IMAGE_LAYER,
		TERRAIN_LAYER,
		FEATURE_LAYER,
		ALL_TYPE
	};

	struct DataGroup {
		QString dataTreeTitle;
		QString objectName;
		QString iconPath;
		QString title;
		QString toolTip;
	};

	QMenu* getOrAddMenu(DataType datType);
	QToolButton* getOrAddToolButton(DataType dataType, QMenu* menu);

	void getFeatureAttribute(const QString& path, QVector<attrib> &attributeList, QStringList &featureFieldList, osgEarth::Symbology::Style* style);
	void addLayerToMap(const QString& path, osgEarth::ModelLayer* layer);
	void addLayerToMap(osg::ref_ptr<osgEarth::Layer> layer, DataType dataType, QString & fileName, QVector<attrib>& attribute, osgEarth::GeoExtent * extent = nullptr);

private:
	// Parse the earth node and record all of its content
	void parseEarthNode();

private:
	static QMenu* dataMenu;
	static QToolBar* dataToolBar;
	static DataGroup _dataGroups[ALL_TYPE];
	static ModelLayerManager* _modelLayerManager;
};
