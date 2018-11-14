#include "AddTMSData.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QInputDialog>

#include <osgEarth/JsonUtils>
#include <osgEarth/XmlUtils>
#include <osgEarth/GeoData>
#include <osgEarth/Registry>
#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>

#include <osgdb_osgearth_heightmap/HeightMapOptions>
#include <osgEarthDrivers/tms/TMSOptions>
using namespace osgEarth;
using namespace osgEarth::Drivers;

static QVector<attrib> getHeightMapInfo(std::string& path, osgEarth::GeoExtent*& extent)
{
	QVector<attrib> attribList;
	char str[1000];

	URI uri(path + "/info.xml");

	XmlDocument* doc = XmlDocument::load(uri, osgDB::Registry::instance()->getOptions());
	if (!doc)
	{
		osg::notify(osg::WARN) << "Failed to fetch attributes!" << std::endl;
		return attribList;
	}

	XmlElement* xmlRoot = doc->getSubElement("info");
	XmlElement* srsElement = xmlRoot->getSubElement("WKT");
	if (!srsElement)
	{
		return attribList;
	}
	std::string wkt;
	wkt = srsElement->getText();
	auto srs = SpatialReference::get(wkt);
	wkt = srs->getWKT();
	attribList.push_back(attrib("wkt", QString(wkt.c_str())));

	osg::notify(osg::NOTICE) << "Got wkt from attribute: " << wkt << std::endl;

	double xmin = 0.0;
	double ymin = 0.0;
	double xmax = 0.0;
	double ymax = 0.0;

	auto extentValue = xmlRoot->getSubElement("Corner_Coordinates");

	if (extentValue)
	{
		attribList.push_back(attrib("Geometry extent", ""));
		QStringList lower_left = QString(extentValue->getSubElement("Lower_Left")->getText().c_str()).split(',');
		QStringList upper_right = QString(extentValue->getSubElement("Upper_Right")->getText().c_str()).split(',');
		xmin = lower_left[0].toDouble();
		ymin = lower_left[1].toDouble();
		xmax = upper_right[0].toDouble();
		ymax = upper_right[1].toDouble();

		sprintf(str, "(%.3lf, %.3lf)", xmin, ymin);
		attribList.push_back(attrib("min", str));
		sprintf(str, "(%.3lf, %.3lf)", xmax, ymax);
		attribList.push_back(attrib("max", str));

		extent = new osgEarth::GeoExtent(srs, xmin, ymin, xmax, ymax);
	}

	return attribList;
}

AddTMSData::AddTMSData()
{
    _pluginCategory = "Data";
    _pluginName = tr("TMS Data");
}

AddTMSData::~AddTMSData()
{

}

void AddTMSData::setupUi(QToolBar *toolBar, QMenu *menu)
{
	QIcon icon;
	icon.addFile(QStringLiteral(":/Atlas/resources/icons/USGSTMS.png"), QSize(), QIcon::Normal, QIcon::Off);

	QAction* addTMSTerAction = new QAction(_mainWindow);
	addTMSTerAction->setObjectName(QStringLiteral("addTMSTerAction"));
	addTMSTerAction->setIcon(icon);
	addTMSTerAction->setText(tr("Online terrain (TMS)"));
	addTMSTerAction->setToolTip(tr("Load online terrain from TMS service"));

	menu = getOrAddMenu(TERRAIN_LAYER);
	menu->addAction(addTMSTerAction);
	connect(addTMSTerAction, SIGNAL(triggered()), this, SLOT(addImage()));

	QAction* addTMSImgAction = new QAction(_mainWindow);
	addTMSImgAction->setObjectName(QStringLiteral("addTMSImgAction"));
	QIcon icon7;
	icon7.addFile(QStringLiteral(":/Atlas/resources/icons/image.png"), QSize(), QIcon::Normal, QIcon::Off);
	addTMSImgAction->setIcon(icon7);
	addTMSImgAction->setText(tr("Online image (TMS)"));
	addTMSImgAction->setToolTip(tr("Load online images from TMS service"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(addTMSImgAction);
	connect(addTMSImgAction, SIGNAL(triggered()), this, SLOT(addImage()));
}

void AddTMSData::addTerrain()
{
	QString fileName = QInputDialog::getText(dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		std::string nodeName = fileName.toLocal8Bit().toStdString();
		QVector<attrib> attribute;
		osgEarth::GeoExtent* extent = NULL;

		HeightMapOptions opt;
		opt.url() = nodeName;
		opt.profile() = osgEarth::Registry::instance()->getGlobalGeodeticProfile()->toProfileOptions();

		opt.format() = "tif";
		opt.invertY() = true;

		auto layer = new ElevationLayer(ElevationLayerOptions(nodeName, opt));
		attribute = getHeightMapInfo(nodeName, extent);

		addLayerToMap(layer, TERRAIN_LAYER, fileName, attribute, extent);
	}
}

void AddTMSData::addImage()
{
	QString fileName = QInputDialog::getText(dynamic_cast<QWidget*>(parent()), tr("Please enter file location"), "");
	if (!fileName.isEmpty())
	{
		std::string nodeName = fileName.toLocal8Bit().toStdString();
		osgEarth::GeoExtent* extent = NULL;
		QVector<attrib> attribute;

		TMSOptions opt;
		opt.url() = nodeName;
		auto layer = new ImageLayer(ImageLayerOptions(nodeName, opt));

		addLayerToMap(layer, IMAGE_LAYER, fileName, attribute, extent);
	}
}
