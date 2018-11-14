#include "EarthDataInterface.h"

#include <DataManager/DataFormats.h>

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <QMessageBox>

#include <osgText/Text>
#include <osg/LineWidth>
#include <osg/Point>

#include <osgEarth/Map>
#include <osgEarth/TerrainLayer>
#include <osgEarth/ModelLayer>
#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>

#include <DataManager/DataManager.h>
#include <SettingsManager/SettingsManager.h>

#include "ModelLayerManager.h"

ModelLayerManager* EarthDataInterface::_modelLayerManager;

QMenu* EarthDataInterface::dataMenu;

QToolBar* EarthDataInterface::dataToolBar;

EarthDataInterface::DataGroup EarthDataInterface::_dataGroups[ALL_TYPE];

EarthDataInterface::EarthDataInterface()
{
    _pluginCategory = "Data";
    _pluginName = tr("osgEarth Data Interface");

	_dataGroups[IMAGE_LAYER] = {
		tr("Image Layers"),
		QStringLiteral("addImage"),
		QStringLiteral("resources/icons/image.png"),
		tr("Image"),
		tr("Add image maps")
	};
	_dataGroups[TERRAIN_LAYER] = {
		tr("Terrain Layers"),
		QStringLiteral("addTerrain"),
		QStringLiteral("resources/icons/terrain.png"),
		tr("Terrain"),
		tr("Add terrain maps")
	};
	_dataGroups[FEATURE_LAYER] = {
		tr("Feature Layers"),
		QStringLiteral("addFeature"),
		QStringLiteral("resources/icons/addshp.png"),
		tr("Feature"),
		tr("Add feature maps")
	};
}

EarthDataInterface::~EarthDataInterface()
{

}

void EarthDataInterface::setupUi(QToolBar *toolBar, QMenu *menu)
{
	dataMenu = menu;
	dataToolBar = toolBar;
	for (unsigned i = 0; i < ALL_TYPE; i++)
	{
		QMenu* menu = getOrAddMenu((DataType)i);
		getOrAddToolButton((DataType)i, menu);
	}

	parseEarthNode();
}

void EarthDataInterface::init()
{
	_modelLayerManager = new ModelLayerManager(getDefaultStyle());

	PluginInterface::init();
}

void EarthDataInterface::showDataAttributes(QString nodeName)
{
	// TODO: Not used yet

	//GDALDataset *poDataset;
	//poDataset = (GDALDataset*)GDALOpenEx(nodeName.toLocal8Bit().toStdString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
	//if (poDataset != NULL)
	//{
	//	QMutex* lock = new QMutex;
	//	lock->lock();
	//	for (int i = 0; i < poDataset->GetLayerCount(); i++)
	//	{

	//		FeatureTableDlg* dlg = new FeatureTableDlg(poDataset, i, lock, (QWidget*)parent());
	//		dlg->setWindowTitle(nodeName + QString(tr(": Layer #")).append(QString::number(i + 1)));
	//		dlg->show();
	//	}
	//	lock->unlock();
	//}
}

QMenu* EarthDataInterface::getOrAddMenu(DataType dataType)
{
	if (dataType >= ALL_TYPE)
		return nullptr;

	const DataGroup & dataGroup = _dataGroups[dataType];

	QMenu* menu = dataMenu->findChild<QMenu*>(dataGroup.objectName + "Menu");
	if (menu)
		return menu;

	menu = new QMenu(dataMenu);
	menu->setObjectName(dataGroup.objectName + "Menu");
	QIcon icon;
	icon.addFile(dataGroup.iconPath, QSize(), QIcon::Normal, QIcon::Off);
	menu->setIcon(icon);
	menu->setTitle(dataGroup.title);
	menu->setToolTip(dataGroup.toolTip);

	dataMenu->addMenu(menu);
	return menu;
}

QToolButton * EarthDataInterface::getOrAddToolButton(DataType dataType, QMenu* menu)
{
	if (dataType >= ALL_TYPE)
		return nullptr;

	const DataGroup & dataGroup = _dataGroups[dataType];

	QToolButton* button = dataMenu->findChild<QToolButton*>(dataGroup.objectName + "Button");
	if (button)
		return button;

	button = new QToolButton(dataToolBar);
	button->setObjectName(dataGroup.objectName + "Button");
	QIcon icon;
	icon.addFile(dataGroup.iconPath, QSize(), QIcon::Normal, QIcon::Off);
	button->setIcon(icon);
	button->setText(dataGroup.title);
	button->setToolTip(dataGroup.toolTip);
	button->setPopupMode(QToolButton::InstantPopup);
	button->setCheckable(true);
	button->setMenu(menu);
	dataToolBar->addWidget(button);
	return button;
}

void EarthDataInterface::getFeatureAttribute(const QString & path, QVector<attrib>& attributeList, QStringList & featureFieldList, osgEarth::Symbology::Style* style)
{
	_modelLayerManager->getFeatureAttribute(path, attributeList, featureFieldList, style);
	_dataManager->updateAttributeList(path, attributeList);
	_dataManager->updateFeatureFieldList(path, featureFieldList);
}

void EarthDataInterface::addLayerToMap(const QString& path, osgEarth::ModelLayer* layer)
{
	for (int i = 0; i < MAX_SUBVIEW; i++)
		_mainMap[i]->addLayer(layer);

	osg::Node *layernode = layer->getOrCreateSceneGraph(_mainMap[0], _mainMap[0]->getReadOptions(), 0L);

	if (!layernode)
	{
		QMessageBox::warning((QWidget*)parent(), tr("Error"), tr("Create node failed!"));
		return;
	}

	emit recordData(layernode, path, tr("Feature Layers"));

	double maxrange = layernode->getBound().radius() * 4;

	layernode->setName(layer->getName());

	layernode->setUserValue("maxrange", maxrange);

	layernode->setUserValue("gemtype", _modelLayerManager->getGemType().toStdString());

	layernode->setUserValue("layerheight", 0);
}

void EarthDataInterface::addLayerToMap(osg::ref_ptr<osgEarth::TerrainLayer> layer, DataType dataType, QString & fileName, QVector<attrib>& attribute, osgEarth::GeoExtent * extent)
{
	if (layer.valid())
	{
		// Try add it to the main map
		_mainMap[0]->addLayer(layer);

		// If success, record the layer and add it to sub maps
		if (((osgEarth::TerrainLayer*)_mainMap[0]->getLayerByName(fileName.toLocal8Bit().toStdString()))->getProfile())
		{
			emit recordData(layer, fileName, _dataGroups[dataType].dataTreeTitle, extent);

			for (int i = 1; i < MAX_SUBVIEW; i++)
				_mainMap[i]->addLayer(layer);

			_dataManager->updateAttributeList(fileName, attribute);
			return;
		}
		else
		{
			_mainMap[0]->removeLayer(layer);
		}
	}

	QMessageBox::warning((QWidget*)parent(), tr("Error"), tr("Data loading failed!"));
}

void EarthDataInterface::parseEarthNode()
{
	osgEarth::LayerVector layers;
	_mainMap[0]->getLayers(layers);
	for each (auto layer in layers)
	{
		QString parent;
		osgEarth::TerrainLayer* terrainLayer = NULL;

		if (!terrainLayer)
		{
			terrainLayer = dynamic_cast<osgEarth::ImageLayer*>(layer.get());
			if (terrainLayer)
				parent = _dataGroups[IMAGE_LAYER].dataTreeTitle;
		}

		if (!terrainLayer)
		{
			terrainLayer = dynamic_cast<osgEarth::ElevationLayer*>(layer.get());
			if (terrainLayer)
				parent = _dataGroups[TERRAIN_LAYER].dataTreeTitle;
		}

		if (terrainLayer)
		{
			std::string originalName = layer->getName();
			QString name = QString::fromStdString(originalName);
			emit recordData(terrainLayer, name, parent);

			// The layer's name may have changed, set its copy's as well
			for (unsigned i = 1; i < MAX_SUBVIEW; i++)
			{
				if (_mainMap[i] != NULL)
				{
					_mainMap[i]->getLayerByName(originalName)->setName(layer->getName());
				}
			}
		}

		return;
	}
}
