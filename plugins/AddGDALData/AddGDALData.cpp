#include "AddGDALData.h"

#include <DataManager/DataFormats.h>

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QFileDialog>

#include <osgEarth/ImageLayer>
#include <osgEarth/ElevationLayer>
#include <osgEarth/ModelLayer>
#include <osgEarthSymbology/Style>

#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/feature_ogr/OGRFeatureOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
using namespace osgEarth;
using namespace osgEarth::Drivers;

#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

static QVector<attrib>  getGDALinfo_Raster(const QString& path)
{
  QVector<attrib>  attribList;
  char             str[1000];
  GDALDataset     *poDataset;
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	CPLSetConfigOption("SHAPE_ENCODING", "");
  poDataset = (GDALDataset *)GDALOpen(path.toLocal8Bit().toStdString().c_str(), GA_ReadOnly);

	if (!poDataset)
  {
    return attribList;
  }

	sprintf(str, "%s/%s",
          poDataset->GetDriver()->GetDescription(),
          poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
	attribList.push_back(attrib("Driver", str));

	sprintf(str, "%dx%dx%d",
          poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
          poDataset->GetRasterCount());
	attribList.push_back(attrib("Size", str));

	if (poDataset->GetProjectionRef() != NULL)
	{
    OGRSpatialReference  srs(poDataset->GetProjectionRef());
    char                *poStr;
		srs.exportToPrettyWkt(&poStr);
		attribList.push_back(attrib("wkt", poStr));
		srs.exportToProj4(&poStr);
		attribList.push_back(attrib("proj", poStr));
		CPLFree(poStr);
	}

  double  adfGeoTransform[6];

	if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None)
	{
		sprintf(str, "(%.6f,%.6f)",
            adfGeoTransform[0], adfGeoTransform[3]);
		attribList.push_back(attrib("Origin", str));
		sprintf(str, "(%.6f,%.6f)",
            adfGeoTransform[1], adfGeoTransform[5]);
		attribList.push_back(attrib("Pixel Size", str));
	}

  GDALRasterBand *poBand;
	int             nBlockXSize, nBlockYSize;
	int             bGotMin, bGotMax;
	double          adfMinMax[2];

	for (int i = 0; i < poDataset->GetRasterCount(); i++)
	{
		sprintf(str, "Raster Band #%d", i + 1);
		attribList.push_back(attrib(str, ""));

		poBand = poDataset->GetRasterBand(i + 1);
		poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);

		sprintf(str, "%dx%d",
            nBlockXSize, nBlockYSize);
		attribList.push_back(attrib("Block", str));

		sprintf(str, "%s",
            GDALGetDataTypeName(poBand->GetRasterDataType()));
		attribList.push_back(attrib("Type", str));

		sprintf(str, "%s",
            GDALGetColorInterpretationName(
              poBand->GetColorInterpretation()));
		attribList.push_back(attrib("ColorInterp", str));

		adfMinMax[0] = poBand->GetMinimum(&bGotMin);
		adfMinMax[1] = poBand->GetMaximum(&bGotMax);

		if (!(bGotMin && bGotMax))
    {
      GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
    }

    sprintf(str, "%.3fd", adfMinMax[0]);
		attribList.push_back(attrib("Min", str));
		sprintf(str, "%.3f", adfMinMax[1]);
		attribList.push_back(attrib("Max", str));

		if (poBand->GetOverviewCount() > 0)
		{
			sprintf(str, "%d",
              poBand->GetOverviewCount());
			attribList.push_back(attrib("Overview Count", str));
		}

		if (poBand->GetColorTable() != NULL)
		{
			sprintf(str, "%d",
              poBand->GetColorTable()->GetColorEntryCount());
			attribList.push_back(attrib(" Color Table Count", str));
		}
	}

	GDALClose(poDataset);

	return attribList;
}

static QVector<attrib>  getGDALinfo_Vector(const QString& path, QVector<feature> &featureTable)
{
  QVector<attrib>  attribList;
  char             str[1000];

	GDALAllRegister();
	OGRRegisterAll();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	CPLSetConfigOption("SHAPE_ENCODING", "");
	GDALDataset *poDataset;
  poDataset = (GDALDataset *)GDALOpenEx(path.toLocal8Bit().toStdString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);

	if (poDataset == NULL)
  {
    return attribList;
  }

	sprintf(str, "%s", poDataset->GetDriver()->GetDescription());
	attribList.push_back(attrib("Driver", str));
	sprintf(str, "%d", poDataset->GetLayerCount());
	attribList.push_back(attrib("Layer count", str));

	for (int i = 0; i < poDataset->GetLayerCount(); i++)
	{
    OGRLayer *poLayer;
		poLayer = poDataset->GetLayer(i);

		sprintf(str, "#%d", i);
		attribList.push_back(attrib("Layer number", str));
		sprintf(str, "%s", poLayer->GetName());
		attribList.push_back(attrib("Layer name", str));
		sprintf(str, "%Id", poLayer->GetFeatureCount());
		attribList.push_back(attrib("Feature count", str));

    char *srs;
		poLayer->GetSpatialRef()->exportToPrettyWkt(&srs);
		sprintf(str, "%s", srs);
		attribList.push_back(attrib("SRS(WKT)", str));
		CPLFree(srs);

    OGREnvelope  psExtent;
		poLayer->GetExtent(&psExtent);
		sprintf(str, "MinX=%.3f\nMinY=%.3f\nMaxX=%.3f\nMaxY=%.3f",
            psExtent.MinX, psExtent.MinY, psExtent.MaxX, psExtent.MaxY);
		attribList.push_back(attrib("Extent", str));

		OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
		sprintf(str, "%s", OGRGeometryTypeToName(poFDefn->GetGeomType()));
		attribList.push_back(attrib("Geom type", str));
		sprintf(str, "%d", poFDefn->GetFieldCount());
		attribList.push_back(attrib("Field count", str));
	}

	GDALClose(poDataset);

	return attribList;
}

AddGDALData::AddGDALData()
{
  _pluginCategory = "Data";
  _pluginName     = tr("GDAL Data");
}

AddGDALData::~AddGDALData()
{
}

void  AddGDALData::setupUi(QToolBar *toolBar, QMenu *menu)
{
  QIcon  icon;

	icon.addFile(QStringLiteral("resources/icons/gdal.png"), QSize(), QIcon::Normal, QIcon::Off);

	// Image
  QAction *addLocalImgAction = new QAction(_mainWindow);
	addLocalImgAction->setObjectName(QStringLiteral("addLocalImgAction"));
	addLocalImgAction->setIcon(icon);
	addLocalImgAction->setText(tr("GDAL (local file)"));
	addLocalImgAction->setToolTip(tr("Load local images with GDAL"));

	menu = getOrAddMenu(IMAGE_LAYER);
	menu->addAction(addLocalImgAction);
	connect(addLocalImgAction, SIGNAL(triggered()), this, SLOT(addImage()));

	// Terrain
  QAction *addLocalTerAction = new QAction(_mainWindow);
	addLocalTerAction->setObjectName(QStringLiteral("addLocalTerAction"));
	addLocalTerAction->setIcon(icon);
	addLocalTerAction->setText(tr("GDAL (local file)"));
	addLocalTerAction->setToolTip(tr("Load local terrain files with GDAL"));

	menu = getOrAddMenu(TERRAIN_LAYER);
	menu->addAction(addLocalTerAction);
	connect(addLocalTerAction, SIGNAL(triggered()), this, SLOT(addTerrain()));

	// Feature
  QAction *addLocalShpAction = new QAction(_mainWindow);
	addLocalShpAction->setObjectName(QStringLiteral("addLocalShpAction"));
	addLocalShpAction->setIcon(icon);
	addLocalShpAction->setText(tr("GDAL (local file)"));
	addLocalShpAction->setToolTip(tr("Load local shapefiles with GDAL"));

	menu = getOrAddMenu(FEATURE_LAYER);
	menu->addAction(addLocalShpAction);
	connect(addLocalShpAction, SIGNAL(triggered()), this, SLOT(addFeature()));
}

void  AddGDALData::addTerrain()
{
  QStringList  fileNames =
    QFileDialog::getOpenFileNames(dynamic_cast<QWidget *>(parent()), tr("Open File"), "", tr("Image File (*.img *.tif *.tiff);;Allfile(*.*)"));

	if (fileNames.isEmpty())
  {
    return;
  }

  emit   loadingProgress(0);
  float  progress = 0;
  float  step     = 100 / fileNames.size();

  for (auto fileName : fileNames)
	{
    std::string  nodeName = fileName.toLocal8Bit().toStdString();

    GDALOptions  opt;
		opt.url() = nodeName;
    osg::ref_ptr<osgEarth::ElevationLayer>  layer = new ElevationLayer(ElevationLayerOptions(nodeName, opt));

    QVector<attrib>  attribute = getGDALinfo_Raster(fileName);

		addLayerToMap(fileName, layer, TERRAIN_LAYER, attribute);

		progress += step;
    emit  loadingProgress(progress);
	}

  emit  loadingDone();
}

void  AddGDALData::addFeature()
{
  QStringList  fileNames = QFileDialog::getOpenFileNames(dynamic_cast<QWidget *>(parent()), tr("Open File"), "", tr("Tiff File (*.shp);;Allfile(*.*)"));

	if (fileNames.isEmpty())
  {
    return;
  }

  emit   loadingProgress(0);
  float  progress = 0;
  float  step     = 100 / fileNames.size();

  for (auto fileName : fileNames)
	{
    std::string  nodeName = fileName.toLocal8Bit().toStdString();

    OGRFeatureOptions  opt;
		opt.url() = nodeName;

    QVector<attrib>             attribList;
    QStringList                 featureFieldList;
    osgEarth::Symbology::Style  style;

		getFeatureAttribute(fileName, attribList, featureFieldList, &style);

    FeatureGeomModelOptions  geomOptions;
    geomOptions.clustering()     = false;
    geomOptions.mergeGeometry()  = true;
		geomOptions.featureOptions() = opt;
    geomOptions.styles()         = new StyleSheet();
		geomOptions.styles()->addStyle(style);
    geomOptions.enableLighting()   = false;
		geomOptions.depthTestEnabled() = false;

    ModelLayerOptions *options = new ModelLayerOptions(nodeName, geomOptions);
    auto               layer   = new ModelLayer(*options);

		addLayerToMap(fileName, layer, FEATURE_LAYER);
		progress += step;
    emit  loadingProgress(progress);
	}

  emit  loadingDone();
}

void  AddGDALData::addImage()
{
  QStringList  fileNames = QFileDialog::getOpenFileNames(
    dynamic_cast<QWidget *>(parent()), tr("Open File"), "", tr("Image File (*.img *.tif *.tiff);;Allfile(*.*)"));

	if (fileNames.isEmpty())
  {
    return;
  }

  emit   loadingProgress(0);
  float  progress = 0;
  float  step     = 100 / fileNames.size();

  for (auto fileName : fileNames)
	{
    std::string  nodeName = fileName.toLocal8Bit().toStdString();

    GDALOptions  opt;
		opt.url() = nodeName;
    osg::ref_ptr<osgEarth::ImageLayer>  layer = new ImageLayer(ImageLayerOptions(nodeName, opt));
		layer->getCacheSettings()->cachePolicy() = osgEarth::CachePolicy::NO_CACHE;

    QVector<attrib>  attribute = getGDALinfo_Raster(fileName);

		addLayerToMap(fileName, layer, IMAGE_LAYER, attribute);

		progress += step;
    emit  loadingProgress(progress);
	}

  emit  loadingDone();
}
