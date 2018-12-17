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
#include <EarthDataInterface/urlDialog.h>

static QVector<attrib>  getWMSInfo(std::string &path, osgEarth::GeoExtent * &extent)
{
  QVector<attrib>              attribList;
  char                         str[1000];
  URI                          uri(path);
  osg::ref_ptr<const Profile>  result;
  char                         sep    = uri.full().find_first_of('?') == std::string::npos ? '?' : '&';
  URI                          capUrl = URI(
    uri.full()
    + sep
    + std::string("service=WMS")
    + std::string("&REQUEST=GetCapabilities"));
  XmlDocument *doc = XmlDocument::load(capUrl.full());

	if (!doc)
	{
		return attribList;
	}

  auto         capabilities = doc->getSubElement("WMS_Capabilities")->getSubElement("Capability");
  auto         rootLayer    = capabilities->getSubElement("Layer");
  XmlNodeList  layers       = rootLayer->getSubElements("Layer");
  std::string  layersStr;

  for (auto layer : layers)
  {
    auto  layerInfo = static_cast<XmlElement *>(layer.get());
    auto  title     = layerInfo->getSubElement("Name");

    if (!title)
    {
      title = layerInfo->getSubElement("name");
    }

    if (!title)
    {
      title = layerInfo->getSubElement("Title");
    }

    if (!title)
    {
      title = layerInfo->getSubElement("title");
    }

    if (title)
    {
      layersStr += title->getText() + ',';
    }

    double  minX, minY, maxX, maxY;
    auto    e_bb = layerInfo->getSubElement("latlonboundingbox");

		if (e_bb)
		{
			minX = as<double>(e_bb->getAttr("minx"), 0);
			minY = as<double>(e_bb->getAttr("miny"), 0);
			maxX = as<double>(e_bb->getAttr("maxx"), 0);
			maxY = as<double>(e_bb->getAttr("maxy"), 0);
		}
		else
		{
      auto  e_gbb = layerInfo->getSubElement("ex_geographicboundingbox");

			if (e_gbb)
			{
				minX = as<double>(e_gbb->getSubElementText("westBoundLongitude"), 0);
				minY = as<double>(e_gbb->getSubElementText("southBoundLatitude"), 0);
				maxX = as<double>(e_gbb->getSubElementText("eastBoundLongitude"), 0);
				maxY = as<double>(e_gbb->getSubElementText("northBoundLatitude"), 0);
			}
		}

    GeoExtent  layerExtent(Registry::instance()->getGlobalGeodeticProfile()->getSRS(), minX, minY, maxX, maxY);

		if (!extent)
    {
      extent = new GeoExtent(layerExtent);
    }
    else
    {
      extent->expandToInclude(layerExtent);
    }
  }

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
  _pluginName     = tr("OGC Data");
}

AddOGCData::~AddOGCData()
{
}

void  AddOGCData::setupUi(QToolBar *toolBar, QMenu *menu)
{
  QIcon  icon;

	icon.addFile(QStringLiteral("resources/icons/ogc.png"), QSize(), QIcon::Normal, QIcon::Off);

  QAction *addOGCImgAction = new QAction(_mainWindow);
	addOGCImgAction->setObjectName(QStringLiteral("addOGCImgAction"));
	addOGCImgAction->setIcon(icon);
	addOGCImgAction->setText(tr("WMS"));
	addOGCImgAction->setToolTip(tr("Load online images from WMS service"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(addOGCImgAction);
	connect(addOGCImgAction, SIGNAL(triggered()), this, SLOT(addImage()));

  QAction *addOGCTerAction = new QAction(_mainWindow);
	addOGCTerAction->setObjectName(QStringLiteral("addOGCTerAction"));
	addOGCTerAction->setIcon(icon);
	addOGCTerAction->setText(tr("WCS"));
	addOGCTerAction->setToolTip(tr("Load online terrain from WCS service"));

	menu = getOrAddMenu(TERRAIN_LAYER);
	menu->addAction(addOGCTerAction);
	connect(addOGCTerAction, SIGNAL(triggered()), this, SLOT(addTerrain()));

  QAction *addOGCShpAction = new QAction(_mainWindow);
	addOGCShpAction->setObjectName(QStringLiteral("addOGCShpAction"));
	addOGCShpAction->setIcon(icon);
	addOGCShpAction->setText(tr("WFS"));
	addOGCShpAction->setToolTip(tr("Load online features from WFS service"));

	menu = getOrAddMenu(FEATURE_LAYER);
	menu->addAction(addOGCShpAction);
	connect(addOGCShpAction, SIGNAL(triggered()), this, SLOT(addFeature()));
}

void  AddOGCData::addTerrain()
{
  QMap<QString, QString>  examples;

  examples["3DEP"] = "https://elevation.nationalmap.gov/arcgis/services/3DEPElevation/ImageServer/WCSServer?";
  urlDialog  dialog(examples, _mainWindow);
  int        accepted = dialog.exec();

  if (accepted == QDialog::Accepted)
  {
    QString  url = dialog.getUrl();

    if (url.isEmpty())
    {
      return;
    }

    auto                 nodeName = url.toLocal8Bit().toStdString();
    osgEarth::GeoExtent *extent   = NULL;

    //// Retrieve available layers
    // auto            attribute = getWMSInfo(nodeName, extent);

    //// Promt for the users to choose layers
    // QStringList     layerNames = attribute.back().second.split(',');
    // MultiChooseDlg  chooseDlg((QWidget *)parent(), layerNames);
    // chooseDlg.exec();
    // QStringList  layersToShow = chooseDlg.getCheckedItems();
    WCSOptions  opt;
    opt.url()        = nodeName;
    opt.format()     = "image/GeoTIFF";
    opt.profile()    = { "EPSG:3857" };
    opt.identifier() = "DEP3ElevationPrototype";

    osg::ref_ptr<osgEarth::ElevationLayer>  layer = new ElevationLayer(ElevationLayerOptions(nodeName, opt));
    auto                                    vec   = QVector<attrib>();
    addLayerToMap(url, layer, TERRAIN_LAYER, vec);
	}
}

void  AddOGCData::addFeature()
{
  QString  fileName = QInputDialog::getText(
    dynamic_cast<QWidget *>(parent()), tr("Please enter file location"), "");

	if (!fileName.isEmpty())
	{
    std::string                 nodeName = fileName.toLocal8Bit().toStdString();
    QVector<attrib>             attribList;
    QStringList                 featureFieldList;
    osgEarth::Symbology::Style  style;

    // Retrieve metadata of the layer
    getFeatureAttribute(fileName, attribList, featureFieldList, &style);

    // TODO: Not robust yet
    MultiChooseDlg  chooseDlg((QWidget *)parent(), featureFieldList);
    chooseDlg.exec();
    QStringList        layersToShow = chooseDlg.getCheckedItems();
    WFSFeatureOptions  opt;
    opt.url()          = nodeName;
		opt.outputFormat() = "GML";
    opt.typeName()     = layersToShow.join(',').toLocal8Bit().toStdString();

    FeatureGeomModelOptions  geomOptions;
		geomOptions.featureOptions() = opt;
    const optional<Config>  con = opt.getConfig();
		geomOptions.styles() = new StyleSheet();
		geomOptions.styles()->addStyle(style);
		geomOptions.enableLighting() = false;
    auto  layer = new ModelLayer(ModelLayerOptions(nodeName, geomOptions));

		addLayerToMap(fileName, layer, FEATURE_LAYER);
	}
}

void  AddOGCData::addImage()
{
  QMap<QString, QString>  examples;

  examples[tr("NEXRAD")] = "http://mesonet.agron.iastate.edu/cgi-bin/wms/nexrad/n0r.cgi";
  urlDialog  dialog(examples, _mainWindow);
  int        accepted = dialog.exec();

  if (accepted == QDialog::Accepted)
  {
    QString  url = dialog.getUrl();

    if (url.isEmpty())
    {
      return;
    }

    auto                 nodeName = url.toLocal8Bit().toStdString();
    osgEarth::GeoExtent *extent   = NULL;

    // Retrieve available layers
    auto  attribute = getWMSInfo(nodeName, extent);

    // Promt for the users to choose layers
    QStringList     layerNames = attribute.back().second.split(',');
    MultiChooseDlg  chooseDlg((QWidget *)parent(), layerNames);
    chooseDlg.exec();
    QStringList  layersToShow = chooseDlg.getCheckedItems();
    WMSOptions   opt;
    opt.url()         = nodeName;
    opt.layers()      = layersToShow.join(',').toLocal8Bit().toStdString();
		opt.transparent() = true;
    opt.format()      = "png";
    opt.profile()     = { "EPSG:4326" };

    auto  layer = new ImageLayer(ImageLayerOptions(nodeName, opt));

		addLayerToMap(url, layer, IMAGE_LAYER, attribute, extent);
	}
}
