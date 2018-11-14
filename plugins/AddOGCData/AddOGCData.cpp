#include "AddOGCData.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QInputDialog>

#include <osgEarth/XmlUtils>
#include <osgEarth/Registry>
#include <osgEarth/ImageLayer>
#include <osgEarth/ModelLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarthSymbology/Style>

#include <osgEarthDrivers/wms/WMSOptions>
#include <osgEarthDrivers/wcs/WCSOptions>
#include <osgEarthDrivers/feature_wfs/WFSFeatureOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
using namespace osgEarth;
using namespace osgEarth::Drivers;

#include "MultiChooseDlg.h"

static QVector<attrib> getWMSInfo(std::string& path, osgEarth::GeoExtent*& extent)
{
	QVector<attrib> attribList;
	char str[1000];

	URI uri(path);
	osg::ref_ptr<const Profile> result;

	char sep = uri.full().find_first_of('?') == std::string::npos ? '?' : '&';

	URI capUrl = URI(
		uri.full() +
		sep +
		std::string("service=WMS") +
		std::string("&REQUEST=GetCapabilities"));

	XmlDocument* doc = XmlDocument::load(capUrl.full());


	if (!doc)
	{
		return attribList;
	}

	XmlElement* capabilities = doc->getSubElement("WMS_Capabilities")->getSubElement("Capability");
	XmlElement* rootLayer = capabilities->getSubElement("Layer");
	XmlNodeList layers = rootLayer->getSubElements("Layer");

	std::string layersStr;

	for each (auto layer in layers)
	{
		XmlElement* layerInfo = static_cast<XmlElement*>(layer.get());
		layersStr += layerInfo->getSubElement("Name")->getText() + ',';

		double minX, minY, maxX, maxY;
		osg::ref_ptr<XmlElement> e_bb = layerInfo->getSubElement("latlonboundingbox");
		if (e_bb.valid())
		{
			minX = as<double>(e_bb->getAttr("minx"), 0);
			minY = as<double>(e_bb->getAttr("miny"), 0);
			maxX = as<double>(e_bb->getAttr("maxx"), 0);
			maxY = as<double>(e_bb->getAttr("maxy"), 0);
		}
		else
		{
			osg::ref_ptr<XmlElement> e_gbb = layerInfo->getSubElement("ex_geographicboundingbox");
			if (e_gbb.valid())
			{
				minX = as<double>(e_gbb->getSubElementText("westBoundLongitude"), 0);
				minY = as<double>(e_gbb->getSubElementText("southBoundLatitude"), 0);
				maxX = as<double>(e_gbb->getSubElementText("eastBoundLongitude"), 0);
				maxY = as<double>(e_gbb->getSubElementText("northBoundLatitude"), 0);
			}
		}

		GeoExtent layerExtent(Registry::instance()->getGlobalGeodeticProfile()->getSRS(), minX, minY, maxX, maxY);
		if (!extent)
			extent = new GeoExtent(layerExtent);
		else
			extent->expandToInclude(layerExtent);
	};

	sprintf(str, "(%.3lf, %.3lf)", extent->xMin(), extent->yMin());
	attribList.push_back(attrib("min", str));
	sprintf(str, "(%.3lf, %.3lf)", extent->xMax(), extent->yMax());
	attribList.push_back(attrib("max", str));

	layersStr.pop_back();
	attribList.push_back(attrib("layers", layersStr.c_str()));

	return attribList;
}

AddOGCData::AddOGCData()
{
    _pluginCategory = "Data";
    _pluginName = tr("OGC Data");
}

AddOGCData::~AddOGCData()
{

}

void AddOGCData::setupUi(QToolBar *toolBar, QMenu *menu)
{
	QIcon icon;
	icon.addFile(QStringLiteral("resources/icons/ogc.png"), QSize(), QIcon::Normal, QIcon::Off);

	QAction* addOGCImgAction = new QAction(_mainWindow);
	addOGCImgAction->setObjectName(QStringLiteral("addOGCImgAction"));
	addOGCImgAction->setIcon(icon);
	addOGCImgAction->setText(tr("Online images (WMS)"));
	addOGCImgAction->setToolTip(tr("Load online images from WMS service"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(addOGCImgAction);
	connect(addOGCImgAction, SIGNAL(triggered()), this, SLOT(addImage()));

	QAction* addOGCTerAction = new QAction(_mainWindow);
	addOGCTerAction->setObjectName(QStringLiteral("addOGCTerAction"));
	addOGCTerAction->setIcon(icon);
	addOGCTerAction->setText(tr("Online terrain (WMS)"));
	addOGCTerAction->setToolTip(tr("Load online terrain from WMS service"));

	menu = getOrAddMenu(TERRAIN_LAYER);
	menu->addAction(addOGCTerAction);
	connect(addOGCTerAction, SIGNAL(triggered()), this, SLOT(addImage()));

	QAction* addOGCShpAction = new QAction(_mainWindow);
	addOGCShpAction->setObjectName(QStringLiteral("addOGCShpAction"));
	addOGCShpAction->setIcon(icon);
	addOGCShpAction->setText(tr("Online feature (WFS)"));
	addOGCShpAction->setToolTip(tr("Load online features from WFS service"));

	menu = getOrAddMenu(FEATURE_LAYER);
	menu->addAction(addOGCShpAction);
	connect(addOGCShpAction, SIGNAL(triggered()), this, SLOT(addFeature()));
}

void AddOGCData::addTerrain()
{
	QString fileName = QInputDialog::getText(dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		auto nodeName = fileName.toLocal8Bit().toStdString();

		WCSOptions opt;
		opt.url() = nodeName;

		auto layer = new ElevationLayer(ElevationLayerOptions(nodeName, opt));
		
		QVector<attrib> attribute;
		addLayerToMap(layer, TERRAIN_LAYER, fileName, attribute);
	}
}

void AddOGCData::addFeature()
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

		WFSFeatureOptions opt;//http://120.26.203.95:8080/geoserver/sl-pipes/ows
		opt.url() = nodeName;
		opt.outputFormat() = "GML";
		opt.typeName() = "sl-pipes:DAOL";

		FeatureGeomModelOptions geomOptions;
		geomOptions.featureOptions() = opt;
		const optional< Config > con = opt.getConfig();
		geomOptions.styles() = new StyleSheet();
		geomOptions.styles()->addStyle(style);
		geomOptions.enableLighting() = false;
		auto layer = new ModelLayer(ModelLayerOptions(nodeName, geomOptions));

		addLayerToMap(fileName, layer);
	}
}

void AddOGCData::addImage()
{
	QString fileName = QInputDialog::getText(dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		auto nodeName = fileName.toLocal8Bit().toStdString();
		osgEarth::GeoExtent* extent = NULL;

		auto attribute = getWMSInfo(nodeName, extent);
		QStringList layerNames = attribute.back().second.split(',');
		MultiChooseDlg chooseDlg((QWidget*)parent(), layerNames);
		chooseDlg.exec();
		QStringList layersToShow = chooseDlg.getCheckedItems();

		WMSOptions opt;
		opt.url() = nodeName;

		opt.layers() = layersToShow.join(',').toLocal8Bit().toStdString();
		opt.transparent() = true;

		opt.format() = "png";

		auto layer = new ImageLayer(ImageLayerOptions(nodeName, opt));

		addLayerToMap(layer, TERRAIN_LAYER, fileName, attribute, extent);
	}
}
