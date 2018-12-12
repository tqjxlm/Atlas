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

#include <osgEarthDrivers/tms/TMSOptions>
using namespace osgEarth;
using namespace osgEarth::Drivers;

#include <EarthDataInterface/urlDialog.h>

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

	QAction* imageAction = new QAction(_mainWindow);
	imageAction->setObjectName(QStringLiteral("addTMSImgAction"));
	imageAction->setText(tr("TMS"));
	imageAction->setToolTip(tr("Load online images from TMS service"));

  QAction* terrainAction = new QAction(_mainWindow);
  terrainAction->setObjectName(QStringLiteral("addTMSTerrainAction"));
  terrainAction->setText(tr("TMS"));
  terrainAction->setToolTip(tr("Load online elevation from TMS service"));

	menu = getOrAddMenu(IMAGE_LAYER);
  menu->addAction(imageAction);

  menu = getOrAddMenu(TERRAIN_LAYER);
  menu->addAction(terrainAction);

	connect(imageAction, SIGNAL(triggered()), this, SLOT(addImage()));
  connect(terrainAction, SIGNAL(triggered()), this, SLOT(addTerrain()));
}

void AddTMSData::addImage()
{
  QMap<QString, QString> examples;
  examples[tr("readymap")] = "http://readymap.org/readymap/tiles/1.0.0/7/";
  urlDialog dialog(examples, _mainWindow);

  int accepted = dialog.exec();
  if (accepted == QDialog::Accepted)
  {
    QString url = dialog.getUrl();
    if (url.isEmpty())
      return;

    std::string nodeName = url.toLocal8Bit().toStdString();
    TMSOptions opt;
    opt.url() = nodeName;

    auto layer = new osgEarth::ImageLayer(osgEarth::ImageLayerOptions(nodeName, opt));

    QVector<attrib> attribute;

    addLayerToMap(url, layer, IMAGE_LAYER, attribute);
  }
}

void AddTMSData::addTerrain()
{
  QMap<QString, QString> examples;
  examples[tr("readymap")] = "http://readymap.org/readymap/tiles/1.0.0/116/";
  urlDialog dialog(examples, _mainWindow);

  int accepted = dialog.exec();
  if (accepted == QDialog::Accepted)
  {
    QString url = dialog.getUrl();
    if (url.isEmpty())
      return;

    std::string nodeName = url.toLocal8Bit().toStdString();
    TMSOptions opt;
    opt.url() = nodeName;

    osg::ref_ptr<osgEarth::ElevationLayer> layer = new osgEarth::ElevationLayer(osgEarth::ElevationLayerOptions(nodeName, opt));

    QVector<attrib> attribute;
    addLayerToMap(url, layer, TERRAIN_LAYER, attribute);
  }
}
