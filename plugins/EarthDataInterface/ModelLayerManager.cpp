#include "ModelLayerManager.h"

#include <QMetaType>

#include <osgText/Text>
#include <osg/LineWidth>
#include <osg/Point>

#include <osgEarthSymbology/Style>
#include <osgEarth/ModelLayer>
#include <osgEarthSymbology/TextSymbol>
#include <osgEarthSymbology/AltitudeSymbol>

// GDAL
#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

#include <osgEarthDrivers/feature_ogr/OGRFeatureOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
using namespace osgEarth;
using namespace osgEarth::Symbology;
using namespace osgEarth::Drivers;

ModelLayerManager::ModelLayerManager(const PluginInterface::StyleConfig& style)
	: _style(style)
{
}

ModelLayerManager::~ModelLayerManager()
{

}

// Read vector meta data using GDAL
QVector<attrib> ModelLayerManager::getVectorMetaData(const QString& path, QStringList &fieldList)
{
	QVector<attrib> attribList;
	char str[1000];

	GDALAllRegister();
	//OGRRegisterAll();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	CPLSetConfigOption("SHAPE_ENCODING", "");
	GDALDataset *poDataset;
	poDataset = (GDALDataset*)GDALOpenEx(path.toLocal8Bit().toStdString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);

	if (poDataset == NULL)
	{
		return attribList;
	}

	// Driver
	sprintf(str, "%s", poDataset->GetDriver()->GetDescription());
	attribList.push_back(attrib("Driver", str));
	sprintf(str, "%d", poDataset->GetLayerCount());
	attribList.push_back(attrib("Layer count", str));

	for (int i = 0; i < poDataset->GetLayerCount(); i++)
	{
		OGRLayer  *poLayer;
		poLayer = poDataset->GetLayer(i);

		// Layer
		sprintf(str, "#%d", i);
		attribList.push_back(attrib("Layer number", str));
		sprintf(str, "%s", poLayer->GetName());
		attribList.push_back(attrib("Layer name", str));
		sprintf(str, "%Id", poLayer->GetFeatureCount());
		attribList.push_back(attrib("Feature count", str));

		// SRS
		char* srs;
		poLayer->GetSpatialRef()->exportToPrettyWkt(&srs);
		sprintf(str, "%s", srs);
		_origlSRS = new char[1000];
		strcpy(_origlSRS, srs);
		attribList.push_back(attrib("SRS(WKT)", str));
		CPLFree(srs);


		// Geo extent
		OGREnvelope psExtent;
		poLayer->GetExtent(&psExtent);
		sprintf(str, "MinX=%.3f\nMinY=%.3f\nMaxX=%.3f\nMaxY=%.3f",
			psExtent.MinX, psExtent.MinY, psExtent.MaxX, psExtent.MaxY);
		attribList.push_back(attrib("Extent", str));

		// Feature
		OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
		sprintf(str, "%s", OGRGeometryTypeToName(poFDefn->GetGeomType()));
		_gemtype = str;
		attribList.push_back(attrib("Geom type", str));
		sprintf(str, "%d", poFDefn->GetFieldCount());
		attribList.push_back(attrib("Field count", str));

		// Attribution
		for (int i = 0; i < poFDefn->GetFieldCount(); i++)
		{
			OGRFieldDefn * fieldogr = poFDefn->GetFieldDefn(i);
			fieldList << QString::fromStdString(fieldogr->GetNameRef());
		}
	}

	GDALClose(poDataset);
	return attribList;
}

void ModelLayerManager::getFeatureAttribute(const QString& path, QVector<attrib> &attributeList, QStringList &featureFieldList, osgEarth::Symbology::Style* style)
{
	// Generate a field list
	QStringList fieldlist;
	attributeList = getVectorMetaData(path, fieldlist);
	featureFieldList = fieldlist;

	if (_gemtype.contains("Line"))
	{
		LineSymbol* ls = style->getOrCreateSymbol<LineSymbol>();
		ls->stroke()->color() = Color(_style.lineColor);
		ls->stroke()->width() = _style.lineWidth->getWidth();
	}
	else if (_gemtype.contains("Point"))
	{
		PointSymbol* ls = style->getOrCreateSymbol<PointSymbol>();
		ls->fill()->color() = Color::Yellow;
		ls->size() = _style.pointSize->getSize();

	}
	else
	{
		PolygonSymbol* ls = style->getOrCreateSymbol<PolygonSymbol>();
		ls->fill()->color() = Color(_style.fillColor);
	}

	{
		AltitudeSymbol* ls = style->getOrCreateSymbol<AltitudeSymbol>();
		ls->clamping() = osgEarth::AltitudeSymbol::CLAMP_RELATIVE_TO_TERRAIN;
		ls->technique() = osgEarth::AltitudeSymbol::TECHNIQUE_DRAPE;
		ls->binding() = osgEarth::AltitudeSymbol::BINDING_VERTEX;
		ls->verticalOffset() = 0;
	}
}

// Vector annotation layer
osg::ref_ptr<osgEarth::ModelLayer> ModelLayerManager::createModelLabelLayer(const QString& layerPath, std::string fieldName, float height)
{
	//StyleConfig _config = _settingmanager->gemStyleConfig("");

	Style labelStyle;

	{
		TextSymbol* text = labelStyle.getOrCreateSymbol<TextSymbol>();

		text->size() = 24.0f;

		text->alignment() = TextSymbol::ALIGN_CENTER_CENTER;

		text->font() = "Resources/simhei.ttf";

		text->fill()->color() = Color::White;

		text->encoding() = TextSymbol::ENCODING_UTF8;

		text->content() = StringExpression(fieldName);

		text->pixelOffset() = osg::Vec2s(0, 20);

	}

	{
		AltitudeSymbol* ls = labelStyle.getOrCreateSymbol<AltitudeSymbol>();
		ls->verticalOffset() = height;
	}

	OGRFeatureOptions opt;

	opt.url() = layerPath.toLocal8Bit().toStdString();

	FeatureGeomModelOptions geomOptions;

	geomOptions.featureOptions() = opt;

	geomOptions.styles() = new StyleSheet();

	geomOptions.styles()->addStyle(labelStyle);

	geomOptions.enableLighting() = false;

	// Vector name + field name = unique label name
	auto labelLayerName = layerPath.toLocal8Bit().toStdString() + "#" + fieldName + "_label";

	osg::ref_ptr<osgEarth::ModelLayer> lableLayer = new ModelLayer(labelLayerName, geomOptions);

	return lableLayer;
}

osg::ref_ptr<osgEarth::ModelLayer> ModelLayerManager::changeLayerStyle(std::string path, const QString& gemtype, std::string iconPath, float layerHeight)
{
	Style style;

	if (gemtype.contains("Line"))
	{
		LineSymbol* ls = style.getOrCreateSymbol<LineSymbol>();
		ls->stroke()->color() = Color(_style.lineColor);
		ls->stroke()->width() = _style.lineWidth->getWidth();
	}
	else if (gemtype.contains("Point"))
	{
		PointSymbol* ls = style.getOrCreateSymbol<PointSymbol>();
		ls->fill()->color() = Color::Yellow;
		ls->size() = _style.pointSize->getSize();

	}
	else if (gemtype.contains("Polygon"))
	{
		PolygonSymbol* ls = style.getOrCreateSymbol<PolygonSymbol>();
		ls->fill()->color() = Color(_style.fillColor);
	}
	else if (gemtype.contains("Icon"))
	{
		IconSymbol* ls = style.getOrCreateSymbol<IconSymbol>();
		ls->url()->setLiteral(iconPath);
		ls->occlusionCull() = false;
		ls->declutter() = false;
	}

	{
		AltitudeSymbol* ls = style.getOrCreateSymbol<AltitudeSymbol>();
		ls->verticalOffset() = layerHeight;
	}


	osg::ref_ptr<osgEarth::ModelLayer> layer;
	OGRFeatureOptions opt;
	opt.url() = path;

	FeatureGeomModelOptions geomOptions;
	geomOptions.clustering() = false;
	geomOptions.mergeGeometry() = true;
	geomOptions.featureOptions() = opt;
	geomOptions.styles() = new StyleSheet();
	geomOptions.styles()->addStyle(style);
	geomOptions.enableLighting() = false;

	ModelLayerOptions* options = new ModelLayerOptions(path, geomOptions);
	layer = new ModelLayer(*options);

	return layer;
}
