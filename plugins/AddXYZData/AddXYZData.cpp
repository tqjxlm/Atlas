#include "AddXYZData.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>

#include <osgEarth/Map>
#include <osgEarth/ImageLayer>
#include <osgEarthDrivers/xyz/XYZOptions>
using namespace osgEarth::Drivers;

#include <EarthDataInterface/urlDialog.h>

AddXYZData::AddXYZData()
{
    _pluginCategory = "Data";
    _pluginName = tr("XYZ Data");
}

AddXYZData::~AddXYZData()
{

}

void AddXYZData::setupUi(QToolBar *toolBar, QMenu *menu)
{
	QAction* imageAction = new QAction(_mainWindow);
	imageAction->setObjectName(QStringLiteral("addXYZImgAction"));
	imageAction->setText(tr("XYZ"));
	imageAction->setToolTip(tr("Load online image with standard XYZ url pattern"));

  QAction* terrainAction = new QAction(_mainWindow);
  terrainAction->setObjectName(QStringLiteral("addXYZTerrainAction"));
  terrainAction->setText(tr("XYZ"));
  terrainAction->setToolTip(tr("Load online terrain with standard XYZ url pattern"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(imageAction);
	connect(imageAction, SIGNAL(triggered()), this, SLOT(addImage()));
}

void AddXYZData::addImage()
{
	QMap<QString, QString> examples;
  examples[tr("Map Box")] = "http://a.tiles.mapbox.com/v4/mapbox.satellite/{z}/{x}/{y}.jpg?access_token=YOUR_TOKEN_HERE";
  examples[tr("Open Street Map")] = "http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png";
	examples[tr("Gaode")] = "http://wprd0[1234].is.autonavi.com/appmaptile?lang=zh_cn&size=1&style=7&x={x}&y={y}&z={z}";
  examples[tr("OpenWeatherMap")] = "http://[abc].tile.openweathermap.org/map/clouds/{z}/{x}/{y}.png";
	urlDialog dialog(examples, _mainWindow);

	int accepted = dialog.exec();
	if (accepted == QDialog::Accepted)
	{
		QString url = dialog.getUrl();
		if (url.isEmpty())
			return;

		std::string nodeName = url.toLocal8Bit().toStdString();
		XYZOptions opt;
		opt.url() = nodeName;
    opt.profile() = { "spherical-mercator" };

		auto layer = new osgEarth::ImageLayer(osgEarth::ImageLayerOptions(nodeName, opt));

		QVector<attrib> attribute;

		addLayerToMap(url, layer, IMAGE_LAYER, attribute);
	}
}

void AddXYZData::addTerrain()
{
  QMap<QString, QString> examples;
  examples[tr("Map Box")] = "http://api.mapbox.com/v4/mapbox.terrain-rgb/{z}/{x}/{y}.pngraw?access_token=YOUR_TOKEN_HERE";
  urlDialog dialog(examples, _mainWindow);

  int accepted = dialog.exec();
  if (accepted == QDialog::Accepted)
  {
    QString url = dialog.getUrl();
    if (url.isEmpty())
      return;

    std::string nodeName = url.toLocal8Bit().toStdString();
    XYZOptions opt;
    opt.url() = nodeName;
    opt.profile() = { "spherical-mercator" };

    if (url.contains("mapbox"))
    {
      opt.elevationEncoding() = "mapbox";
    }

    auto layer = new osgEarth::ElevationLayer(osgEarth::ElevationLayerOptions(nodeName, opt));

    QVector<attrib> attribute;

    addLayerToMap(url, layer, TERRAIN_LAYER, attribute);
  }
}
