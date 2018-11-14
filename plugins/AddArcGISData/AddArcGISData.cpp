#include "AddArcGISData.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QInputDialog>

#include <osgDb/Registry>

#include <osgEarth/JsonUtils>
#include <osgEarth/XmlUtils>
#include <osgEarth/GeoData>
#include <osgEarth/Profile>

#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarth/ModelLayer>
#include <osgEarthSymbology/Style>
#include <osgdb_osgearth_arcgis/ArcGISOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>

using namespace osgEarth;
using namespace osgEarth::Drivers;

static QVector<attrib> getArcGISInfo(std::string& path, osgEarth::GeoExtent*& extent)
{
	QVector<attrib> attribList;
	char str[1000];

	URI uri(path);
	std::string sep = uri.full().find("?") == std::string::npos ? "?" : "&";
	std::string json_url = uri.full() + sep + std::string("f=pjson");

	ReadResult r = URI(json_url).readString(osgDB::Registry::instance()->getOptions());
	if (r.failed())
	{
		return attribList;
	}

	Json::Value doc;
	Json::Reader reader;

	if (!reader.parse(r.getString(), doc))
	{
		return attribList;
	}

	attribList.push_back(attrib("currentVersion", doc["currentVersion"].asCString()));
	attribList.push_back(attrib("units", doc["units"].asCString()));
	for each (Json::Value layer in doc["layers"])
	{
		attribList.push_back(attrib(("layer #" + layer["id"].asString()).c_str(), layer["name"].asCString()));
	};

	double xmin = 0.0;
	double ymin = 0.0;
	double xmax = 0.0;
	double ymax = 0.0;
	int srs = 0;

	Json::Value fullExtentValue = doc["fullExtent"];
	std::string srsValue;

	if (!fullExtentValue.empty())
	{
		attribList.push_back(attrib("fullExtent", ""));
		xmin = doc["fullExtent"].get("xmin", 0).asDouble();
		ymin = doc["fullExtent"].get("ymin", 0).asDouble();
		xmax = doc["fullExtent"].get("xmax", 0).asDouble();
		ymax = doc["fullExtent"].get("ymax", 0).asDouble();

		sprintf(str, "(%.3lf, %.3lf)", xmin, ymin);
		attribList.push_back(attrib("min", str));
		sprintf(str, "(%.3lf, %.3lf)", xmax, ymax);
		attribList.push_back(attrib("max", str));

		srs = doc["fullExtent"].get("spatialReference", osgEarth::Json::Value::null).get("latestWkid", 0).asInt();
		if (srs == 0)
			srs = doc["fullExtent"].get("spatialReference", osgEarth::Json::Value::null).get("wkid", 0).asInt();
		if (srs != 0)
		{
			sprintf(str, "%d", srs);
			attribList.push_back(attrib("srs", str));
		}

		srsValue = doc["fullExtent"].get("spatialReference", osgEarth::Json::Value::null).get("wkt", "null").asString();
		attribList.push_back(attrib("wkt", QString(srsValue.c_str())));

		extent = new osgEarth::GeoExtent(osgEarth::SpatialReference::get("epsg:" + std::to_string(srs)), xmin, ymin, xmax, ymax);
	}

	return attribList;
}

AddArcGISData::AddArcGISData()
{
    _pluginCategory = "Data";
    _pluginName = tr("ArcGIS Data");
}

AddArcGISData::~AddArcGISData()
{

}

void AddArcGISData::setupUi(QToolBar * toolBar, QMenu * menu)
{
	QIcon icon;
	icon.addFile(QStringLiteral("resources/icons/ArcGIS.png"), QSize(), QIcon::Normal, QIcon::Off);

	// Image
	QAction* addArcGISImgAction = new QAction(_mainWindow);
	addArcGISImgAction->setObjectName(QStringLiteral("addArcGISImgAction"));
	addArcGISImgAction->setIcon(icon);
	addArcGISImgAction->setText(tr("Online image (ArcGIS)"));
	addArcGISImgAction->setToolTip(tr("Load online images from ArcGIS service"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(addArcGISImgAction);
	connect(addArcGISImgAction, SIGNAL(triggered()), this, SLOT(addImage()));

	// Terrain
	QAction* addArcGISTerAction = new QAction(_mainWindow);
	addArcGISTerAction->setObjectName(QStringLiteral("addArcGISTerAction"));
	addArcGISTerAction->setIcon(icon);
	addArcGISTerAction->setText(tr("Online terrain (ArcGIS)"));
	addArcGISTerAction->setToolTip(tr("Load online terrain from ArcGIS service"));

	menu = getOrAddMenu(TERRAIN_LAYER);
	menu->addAction(addArcGISTerAction);
	connect(addArcGISTerAction, SIGNAL(triggered()), this, SLOT(addImage()));

	// Feature
	QAction* addArcGISShpAction = new QAction(_mainWindow);
	addArcGISShpAction->setObjectName(QStringLiteral("addArcGISShpAction"));
	addArcGISShpAction->setIcon(icon);
	addArcGISShpAction->setText(tr("Online features (ArcGIS)"));
	addArcGISShpAction->setToolTip(tr("Load online features from ArcGIS service"));

	menu = getOrAddMenu(FEATURE_LAYER);
	menu->addAction(addArcGISShpAction);
	connect(addArcGISShpAction, SIGNAL(triggered()), this, SLOT(addFeature()));
}

void AddArcGISData::addImage()
{
	QString fileName = QInputDialog::getText(dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		auto nodeName = fileName.toLocal8Bit().toStdString();
		osgEarth::GeoExtent* extent = NULL;

		QVector<attrib> attribute = getArcGISInfo(nodeName, extent);
		ArcGISOptions opt;
		opt.url() = nodeName;
		ImageLayerOptions layerOpt(nodeName, opt);
		layerOpt.minFilter() = osg::Texture::LINEAR_MIPMAP_LINEAR;
		layerOpt.magFilter() = osg::Texture::LINEAR;
		osg::ref_ptr<osgEarth::ImageLayer> layer = new ImageLayer(layerOpt);

		addLayerToMap(layer, IMAGE_LAYER, fileName, attribute, extent);
	}
}

void AddArcGISData::addTerrain()
{
	QString fileName = QInputDialog::getText(dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		auto nodeName = fileName.toLocal8Bit().toStdString();
		osgEarth::GeoExtent* extent = NULL;

		ArcGISOptions opt;
		opt.url() = nodeName;

		osg::ref_ptr<osgEarth::ElevationLayer> layer = new ElevationLayer(ElevationLayerOptions(nodeName, opt));
		QVector<attrib> attribute = getArcGISInfo(nodeName, extent);

		addLayerToMap(layer, TERRAIN_LAYER, fileName, attribute, extent);
	}
}

void AddArcGISData::addFeature()
{
	QString	fileName = QInputDialog::getText(
		dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		std::string nodeName = fileName.toLocal8Bit().toStdString();

		QVector<attrib> attribList;
		QStringList featureFieldList;
		osgEarth::Symbology::Style style;

		getFeatureAttribute(fileName, attribList, featureFieldList, &style);

		ArcGISOptions opt;
		opt.url() = nodeName;
		ArcGISOptions arcgisonline;
		arcgisonline.url() = osgEarth::URI(nodeName.c_str());
		osgEarth::Drivers::FeatureGeomModelOptions arcGeomOptions;
		arcGeomOptions.featureOptions() = arcgisonline;
		arcGeomOptions.styles() = new StyleSheet();
		arcGeomOptions.styles()->addStyle(style);
		arcGeomOptions.enableLighting() = false;
		auto layer = new ModelLayer(ModelLayerOptions(nodeName, arcGeomOptions));

		addLayerToMap(fileName, layer);
	}
}
