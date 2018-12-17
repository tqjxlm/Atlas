#include "DrawVector.h"

#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDebug>
#include <QAction>
#include <QToolBar>
#include <QMenu>

#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osgText/Text>
#include <osgSim/OverlayNode>
#include <osgViewer/View>
#include <osgUtil/Tessellator>

#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>

#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

#include <ViewerWidget/ViewerWidget.h>

static QMap<int, QString>    fieldMap;
static QMap<int, osg::Vec3>  geomMap;

DrawVector::DrawVector()
{
  _pluginName     = tr("Vector");
	_pluginCategory = "Draw";

	_vectorLod = NULL;

	_highestVisibleHeight = 0;
}

DrawVector::~DrawVector()
{
}

void  DrawVector::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("drawVectorAciton"));
	_action->setCheckable(true);
	_action->setVisible(false);
}

void  DrawVector::OpenVectorFile(const QString &path)
{
  QString  nameLine;
  QString  nameN, nameX, nameY;
  QFile    file(path);

	if (file.open(QFile::ReadOnly | QIODevice::Text))
	{
    QTextStream  data(&file);

		while (!data.atEnd())
		{
			nameLine = data.readLine();
      QByteArray  cname = nameLine.section(",", 0, 0).toLocal8Bit();
			nameX = nameLine.section(",", 1, 1);
			nameY = nameLine.section(",", 2, 2);
      osg::Vec3  pos = osg::Vec3(nameX.toFloat(), nameY.toFloat(), 0);

			if (!_vectorLod)
			{
				_vectorLod = new osg::LOD;
				_drawVectorRoot->addChild(_vectorLod);
			}

      if (!_currentDrawNode)
			{
				_currentDrawNode = new osg::Geode();
				_vectorLod->addChild(_currentDrawNode);
			}

      // 以linesegment求交
      osgUtil::LineSegmentIntersector::Intersections  intersections;

      // 从水平面点的高程z=1000处到z=-300处
      osg::ref_ptr<osgUtil::LineSegmentIntersector>  ls = new osgUtil::LineSegmentIntersector
                                                            (osg::Vec3(pos.x(), pos.y(), 1000),
                                                            osg::Vec3(pos.x(), pos.y(), -300));
      // 对overlayNode求交
      osg::ref_ptr<osgUtil::IntersectionVisitor>  iv = new osgUtil::IntersectionVisitor(ls);
			_overlayNode->accept(*iv);

      // 输出所有的交点
			if (ls->containsIntersections())
			{
				intersections = ls->getIntersections();

        for (osgUtil::LineSegmentIntersector::Intersections::iterator iter = intersections.begin(); iter != intersections.end(); iter++)
				{
          osg::Vec3  first_intersection = iter->getWorldIntersectPoint();

					pos.z() = first_intersection.z();

          break;// 求第一个交点即break结束
				}
			}

      pos = osg::Vec3(pos.x() - _anchoredOffset.x(), pos.y() - _anchoredOffset.y(), pos.z());

			_currentDrawNode->addDrawable(createPointGeode(pos));

      auto  str = cname.toStdString();
      _currentDrawNode->addDrawable(createTextGeode(str, pos));
    }

    recordNode(_vectorLod, path);
	}
}

void  DrawVector::convtGeoCoorToProCoor(osg::Vec3 &proPos, osg::Vec3 geoPos)
{
  // 将点从地理坐标转换为世界坐标
  osgEarth::SpatialReference *srs_wgs84 = osgEarth::SpatialReference::get(_origlSRS);
  osgViewer::View            *view      = _mainViewer->getMainView();
  osgEarth::GeoPoint          aimPos    = osgEarth::GeoPoint(srs_wgs84, geoPos).transform(_globalSRS);

	proPos.x() = aimPos.x();

	proPos.y() = aimPos.y();

	proPos.z() = aimPos.z();
}

osg::Vec3  DrawVector::MiddlePointOfPolygon(osg::ref_ptr<osg::Vec3Array> pointsArr, osg::Vec3 centerpoint)
{
  osg::Vec3  pointMiddle;
  float      mindistance = 10000;

	for (int i = 0; i < pointsArr->size(); i++)
	{
    float  dis = (pointsArr->at(i) - centerpoint).length();

		if (dis < mindistance)
		{
			mindistance = dis;
			pointMiddle = pointsArr->at(i);
		}
	}

  return pointMiddle;
}

osg::ref_ptr<osg::Geometry>  DrawVector::createPolyGem(osg::ref_ptr<osg::Vec3Array> polygonArr)
{
	if (polygonArr->size() > 0)
	{
    osg::ref_ptr<osg::Geometry>  polyGeom = new osg::Geometry();

		polyGeom->setVertexArray(polygonArr);

    osg::ref_ptr<osg::Vec4Array>  colors = new osg::Vec4Array;
		colors->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));

		polyGeom->setColorArray(colors.get(), osg::Array::BIND_OVERALL);

    osg::ref_ptr<osg::Vec3Array>  normals = new osg::Vec3Array;
		normals->push_back(osg::Vec3(0.f, 0.f, 1.f));

		polyGeom->setNormalArray(normals.get(), osg::Array::BIND_OVERALL);

		polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, polygonArr->size()));

    osg::ref_ptr<osgUtil::Tessellator>  tessellator = new osgUtil::Tessellator();

		tessellator->setTessellationType(osgUtil::Tessellator::TESS_TYPE_GEOMETRY);

		tessellator->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);

		tessellator->setTessellationNormal(osg::Vec3d(0, 0, 1.0));

    // tessellator->setBoundaryOnly(true);
		tessellator->retessellatePolygons(*(polyGeom));

		return polyGeom;
	}

  return NULL;
}

osg::ref_ptr<osg::Geometry>  DrawVector::createLineGem(osg::ref_ptr<osg::Vec3Array> polygonArr)
{
	if (polygonArr->size() > 0)
	{
    osg::ref_ptr<osg::Geometry>  lineGeom = new osg::Geometry();
		lineGeom->setVertexArray(polygonArr);

    osg::ref_ptr<osg::Vec4Array>  colors = new osg::Vec4Array;
		colors->push_back(lineColor);

		lineGeom->setColorArray(colors, osg::Array::BIND_OVERALL);

		lineGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, polygonArr->size()));

    osg::ref_ptr<osg::LineWidth>  LineSize = new osg::LineWidth;

		LineSize->setWidth(2.0);

		lineGeom->getOrCreateStateSet()->setAttributeAndModes(LineSize.get(), osg::StateAttribute::ON);

		return lineGeom;
	}

  return NULL;
}

osg::ref_ptr<osg::Geode>  DrawVector::drawPolygon(OGRPolygon *poly, osg::Vec3 &centerpoint)
{
  osg::ref_ptr<osg::Geode>  geode = new osg::Geode();
  OGRwkbGeometryType        targetGeometryType;
  int                       innerCount = poly->getNumInteriorRings();

  if (innerCount == 0)// 如果没有内环
	{
    osg::ref_ptr<osg::Vec3Array>  polygonArr = new osg::Vec3Array;

		targetGeometryType = OGRwkbGeometryType::wkbLineString;

    OGRLineString *pOGRLineString = (OGRLineString *)OGRGeometryFactory::createGeometry(targetGeometryType);
    OGRLinearRing *pOGRLinearRing = poly->getExteriorRing();
    int            pointCount     = pOGRLinearRing->getNumPoints();

		for (int i = 0; i < pointCount; i++)
		{
      osg::Vec3  vecPoint;
			vecPoint.x() = pOGRLinearRing->getX(i);
			vecPoint.y() = pOGRLinearRing->getY(i);
			vecPoint.z() = pOGRLinearRing->getZ(i);
      // vecPoint *= 1000;
      osg::Vec3  posAim;
			convtGeoCoorToProCoor(posAim, vecPoint);
			posAim = osg::Vec3(posAim.x(), posAim.y(), 10);
			polygonArr->push_back(posAim);
			centerpoint += posAim;
		}

		centerpoint = centerpoint / pointCount;

		geode->addDrawable(createLineGem(polygonArr));
  }
	else
	{
    osg::ref_ptr<osg::Vec3Array>  polygonArr = new osg::Vec3Array;
		targetGeometryType = OGRwkbGeometryType::wkbMultiLineString;
    int  countpoint = 0;

    for (int i = 0; i < innerCount; i++)// 遍历每个内环
		{
      osg::ref_ptr<osg::Vec3Array>  polygonArr2 = new osg::Vec3Array;
			// 添加内环
      OGRLineString  ogrLineString0;
      OGRLinearRing *pOGRLinearRing0 = poly->getInteriorRing(i);
      int            pointCount      = pOGRLinearRing0->getNumPoints();
			countpoint += pointCount;

      for (int i = 0; i < pointCount; i++)
			{
        osg::Vec3  vecPoint;
				vecPoint.x() = pOGRLinearRing0->getX(i);
				vecPoint.y() = pOGRLinearRing0->getY(i);
				vecPoint.z() = pOGRLinearRing0->getZ(i);
        // vecPoint *= 1000;
        osg::Vec3  posAim;
        convtGeoCoorToProCoor(posAim, vecPoint);
				posAim = osg::Vec3(posAim.x(), posAim.y(), 10);
				polygonArr2->push_back(posAim);
				centerpoint += posAim;
			}

      // geode->addDrawable(createPolyGem(polygonArr2));
			geode->addDrawable(createLineGem(polygonArr2));
		}

    // 添加外环
    OGRLineString  ogrLineString;
    OGRLinearRing *pOGRLinearRing = poly->getExteriorRing();
    int            pointCount     = pOGRLinearRing->getNumPoints();
		countpoint += pointCount;
    double  x = 0;
    double  y = 0;

		for (int i = 0; i < pointCount; i++)
		{
      osg::Vec3  vecPoint;
			vecPoint.x() = pOGRLinearRing->getX(i);
			vecPoint.y() = pOGRLinearRing->getY(i);
			vecPoint.z() = pOGRLinearRing->getZ(i);
      // vecPoint *= 1000;
      osg::Vec3  posAim;
      convtGeoCoorToProCoor(posAim, vecPoint);
			posAim = osg::Vec3(posAim.x(), posAim.y(), 10);
			polygonArr->push_back(posAim);
			centerpoint += posAim;
		}

    // geode->addDrawable(createPolyGem(polygonArr));
		geode->addDrawable(createLineGem(polygonArr));
		centerpoint = centerpoint / countpoint;
		centerpoint = MiddlePointOfPolygon(polygonArr, centerpoint);
	}

	return geode;
}

osg::ref_ptr<osg::Geometry>  DrawVector::drawLineString(OGRLineString *linestring)
{
  int                           pointCount = linestring->getNumPoints();
  osg::ref_ptr<osg::Vec3Array>  plinearray = new osg::Vec3Array;

	for (int i = 0; i < pointCount; i++)
	{
    osg::Vec3  vecPoint;
		vecPoint.x() = linestring->getX(i);
		vecPoint.y() = linestring->getY(i);
		vecPoint.z() = linestring->getZ(i);
    osg::Vec3  posAim;
		convtGeoCoorToProCoor(posAim, vecPoint);
		posAim = osg::Vec3(posAim.x(), posAim.y(), 10);
		plinearray->push_back(posAim);
	}

	return createLineGem(plinearray);
}

void  DrawVector::DrawVectorFromFile(QString filePath, QString nodeName)
{
	if (filePath.contains("\\"))
	{
		filePath.replace("\\", "//");
	}

	_vecFontSize = 40.0;

	fieldMap.clear();

	geomMap.clear();

  QMap<int, QString>  nameMap;
  QMap<int, QString>  areaMap;

  // nodeName = "中国";

  // filePath = QFileDialog::getOpenFileName(_dataManager->getMainProgram()->getMainViewer(), tr("Load vector"), " ", tr("vector data(*.shp);;Allfile(*.*);"));

	_vectorLod = new osg::LOD;

	_drawVectorRoot->addChild(_vectorLod);

	_vectorLod->setName(nodeName.toLocal8Bit());

	_labelGroup = new osg::Group;

	_labelGroup->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

	_labelGroup->getOrCreateStateSet()->setRenderBinDetails(2, "RenderBin");

	_drawVectorRoot->addChild(_labelGroup);

  osg::ref_ptr<osg::Group>  vectorGroup = new osg::Group;

	_currentDrawNode = new osg::Geode();

	nodeName = nodeName + "_label";

	_labelGroup->setName(nodeName.toLocal8Bit());

  // 获得文件名
  QString  name = filePath.right(filePath.length() - filePath.lastIndexOf("/") - 1);
	name = name.left(name.indexOf("."));

  GDALDataset *poDS;
  poDS = (GDALDataset *)GDALOpenEx(filePath.toLocal8Bit().toStdString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);

	if (poDS == NULL)
	{
		printf("Open failed.\n");

    return;
	}

  // 打开图层
  OGRLayer *poLayer;

	poLayer = poDS->GetLayerByName(name.toLocal8Bit().toStdString().c_str());

  // 获得坐标信息
  // char* srs;
	poLayer->GetSpatialRef()->exportToPrettyWkt(&_origlSRS);

	OGRFeature *poFeature;

	poLayer->ResetReading();

	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
  int             iField;
  int             fieldCount = poFDefn->GetFieldCount();
  int             gemtype    = 0;
  int             uniqueID   = 0;

	while ((poFeature = poLayer->GetNextFeature()) != NULL)
	{
		uniqueID++;
    // 获取属性信息
    QMap<QString, QString>  field;

		for (iField = 0; iField < fieldCount; iField++)
		{
			OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(iField);
			field.insert(poFieldDefn->GetNameRef(), poFeature->GetFieldAsString(iField));
		}

    // 获取几何信息
		OGRGeometry *poGeometry;

		poGeometry = poFeature->GetGeometryRef();

		if (poGeometry != NULL)
		{
			switch (wkbFlatten(poGeometry->getGeometryType()))
			{
			case wkbMultiPolygon:
			{
        // OGRMultiPolygon *polygons = (OGRMultiPolygon*)poGeometry;

        // int nn = polygons->getNumGeometries();//计算每个feature下有几个多边形

        // for (int ii = 0; ii < nn; ii++)//遍历每个多边形
        // {
        // OGRPolygon *poly = (OGRPolygon*)polygons->getGeometryRef(ii);

        // osg::Vec3 centerpoint;

        // osg::ref_ptr<osg::Geode> geode = drawPolygon(poly, centerpoint);

        // _drawVectotRoot->addChild(geode);
        // }
			}
			break;
			case wkbPolygon:
			{
        osg::ref_ptr<osg::Geode>  geode;
        OGRPolygon               *polygon = (OGRPolygon *)poGeometry;
        osg::Vec3                 centerpoint;

				if (name == "ChangXing_scope_farmerhouse")
				{
          QString  sname        = "建筑面积";
          QString  polyarea     = QString::number(atof(field.value(sname).toStdString().c_str()), 'f', 1);
          QString  farmarea     = QString::number(atof(field.value("F_AREA").toStdString().c_str()), 'f', 1);
          QString  polyareaflag = "s";
          QString  farmareaflag = "f";
          QString  polyname     = field.value("FNAME");
          QString  connecStr    = polyareaflag + polyarea + "\n" + farmareaflag + farmarea;

          fieldMap.insert(uniqueID, "");// 以feature唯一id作为key，添加要素属性到map
					nameMap.insert(uniqueID, polyname);
					areaMap.insert(uniqueID, connecStr);
				}
				else if (name == "changxingAdminiScope")
				{
					fieldMap.insert(uniqueID, "");
				}

				styleSetting(name);

				geode = drawPolygon(polygon, centerpoint);

				centerpoint += osg::Vec3(0, 0, 0);

        geomMap.insert(uniqueID, centerpoint);// 以feature唯一id作为key，添加要素图形到map

				vectorGroup->addChild(geode);
			}
			break;
			case wkbPoint:
			{
        OGRPoint  *point = (OGRPoint *)poGeometry;
        osg::Vec3  pos;

				convtGeoCoorToProCoor(pos, osg::Vec3(point->getX(), point->getY(), point->getZ()));

        osgUtil::LineSegmentIntersector::Intersections  intersections;
        osg::ref_ptr<osgUtil::LineSegmentIntersector>   ls = new osgUtil::LineSegmentIntersector
                                                               (osg::Vec3(pos.x(), pos.y(), 1000),
                                                               osg::Vec3(pos.x(), pos.y(), -300)); // 从水平面点的高程z=1000处到z=-300处
        osg::ref_ptr<osgUtil::IntersectionVisitor>  iv = new osgUtil::IntersectionVisitor(ls);

        _overlayNode->accept(*iv);// 对overlayNode求交

        // 输出所有的交点
				if (ls->containsIntersections())
				{
					intersections = ls->getIntersections();

          for (osgUtil::LineSegmentIntersector::Intersections::iterator iter = intersections.begin(); iter != intersections.end(); iter++)
					{
            osg::Vec3  first_intersection = iter->getWorldIntersectPoint();

						pos.z() = first_intersection.z();

            break;// 求第一个交点即break结束
					}
				}

        QString  sthvalue = field.value("FNAME");

				fieldMap.insert(uniqueID, sthvalue);

        geomMap.insert(uniqueID, pos);// 以feature唯一id作为key，添加要素图形到map

				styleSetting(name);

				_currentDrawNode->addDrawable(createPointGeode(pos));

				vectorGroup->addChild(_currentDrawNode);
			}
			break;
			case wkbLineString:
			{
        osg::ref_ptr<osg::Geode>  geode;
        OGRLineString            *plinestring = (OGRLineString *)poGeometry;
        osg::Vec3                 centerpoint;

				if (name == "oldHouseScope")
				{
					fieldMap.insert(uniqueID, "");
				}

				styleSetting(name);

				_currentDrawNode->addDrawable(drawLineString(plinestring));

        geomMap.insert(uniqueID, centerpoint);// 以feature唯一id作为key，添加要素图形到map

				vectorGroup->addChild(_currentDrawNode);
			}
			break;
			}
		}
	}

  lodSetting(0, vectorGroup, name);

	if (name == "ChangXing_scope_farmerhouse")
	{
    osg::ref_ptr<osg::Group>  extraLabelGroup2 = new osg::Group;

		extraLabelGroup2->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

		extraLabelGroup2->getOrCreateStateSet()->setRenderBinDetails(2, "RenderBin");

		_drawVectorRoot->addChild(extraLabelGroup2);

		extraLabelGroup2->setNodeMask(0);

    QString  areanamee = "农居房面积";
		extraLabelGroup2->setName(areanamee.toLocal8Bit());

		createFeatureNoteText(areaMap, geomMap, extraLabelGroup2);

    osg::ref_ptr<osg::Group>  extraLabelGroup1 = new osg::Group;

		extraLabelGroup1->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

		extraLabelGroup1->getOrCreateStateSet()->setRenderBinDetails(2, "RenderBin");

		_drawVectorRoot->addChild(extraLabelGroup1);
		extraLabelGroup1->setNodeMask(0);

		areanamee = "农居房名称";

		extraLabelGroup1->setName(areanamee.toLocal8Bit());

    QMapIterator<int, osg::Vec3>  iter(geomMap);

		while (iter.hasNext())
		{
			iter.next();
      osg::Vec3  pos = iter.value();
			pos.y() -= 30;
			geomMap.insert(iter.key(), pos);
		}

    createFeatureNoteText(nameMap, geomMap, extraLabelGroup1);
  }

	createFeatureNoteText(fieldMap, geomMap, _labelGroup);

  // 图层名称更新
	QTextCodec *codec = QTextCodec::codecForName("GB2312");
  QString     ss    = codec->toUnicode(_vectorLod->getName().c_str());

	recordNode(_vectorLod, ss);

	GDALClose(poDS);
}

void  DrawVector::lodSetting(double dis, osg::Group *vectorGroup, const QString &name)
{
	if (name == "ChangXing_scope_farmerhouse")
	{
    _vectorLod->addChild(vectorGroup, 0, vectorGroup->getBound().radius() * 4.0);
    qDebug() << vectorGroup->getBound().radius() * 4.0 << endl;
		_highestVisibleHeight = 1738;
		_labelGroup->setNodeMask(0);
	}
	else if (name == "changxingAdminiScope")
	{
		_vectorLod->addChild(vectorGroup, 0, 54262);
		_highestVisibleHeight = 54262;
		_labelGroup->setNodeMask(1);
	}
	else if (name == "oldHouseScope")
	{
		_vectorLod->addChild(vectorGroup, 0, 18198.1);
		_highestVisibleHeight = 1738;
		_labelGroup->setNodeMask(1);
	}
	else if (name == "townPoint")
	{
		_vectorLod->addChild(vectorGroup, 0, 54262);
		_highestVisibleHeight = 54262;
		_labelGroup->setNodeMask(1);
	}
	else if (name == "adminiCountyPoint")
	{
		_vectorLod->addChild(vectorGroup, 0, 28800);
		_highestVisibleHeight = 28800;
		_labelGroup->setNodeMask(1);
	}
	else if (name == "countyNature")
	{
		_vectorLod->addChild(vectorGroup, 0, 13704);
		_highestVisibleHeight = 13704;
		_labelGroup->setNodeMask(0);
	}
}

void  DrawVector::styleSetting(const QString &name)
{
	if (name == "ChangXing_scope_farmerhouse")
	{
    lineColor          = osg::Vec4(1.0, 1.0, 190.0 / 255.0, 1.0);
    _textColor         = osg::Vec4(1.0, 1.0, 0.0, 1.0);
		_textCharactorMode = osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT;
	}
	else if (name == "changxingAdminiScope")
	{
    _textColor         = osg::Vec4(1.0, 0.0f, 0.0, 1);
    lineColor          = osg::Vec4(64.0f / 255, 101.0f / 255, 235.0f / 255, 1);
		_textCharactorMode = osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT;
	}
	else if (name == "oldHouseScope")
	{
    _textColor         = osg::Vec4(1.0, 0.0f, 0.0, 1);
    lineColor          = osg::Vec4(1.0, 0.0f, 0.0, 1);
		_textCharactorMode = osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT;
	}
	else if (name == "townPoint")
	{
    _pointColor        = osg::Vec4(230.0f / 255, 0.0f / 255, 169.0f / 255, 1);
    _textColor         = osg::Vec4(230.0f / 255, 0.0f / 255, 169.0f / 255, 1);
		_textCharactorMode = osgText::Text::SCREEN_COORDS;
	}
	else if (name == "adminiCountyPoint")
	{
    _pointColor        = osg::Vec4(0.0, 1.0, 0, 1);
    _textColor         = osg::Vec4(0.0, 1.0, 0, 1);
		_textCharactorMode = osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT;
	}
	else if (name == "countyNature")
	{
    _pointColor        = osg::Vec4(1.0, 1.0, 0, 1);
    _textColor         = osg::Vec4(1.0, 1.0, 0, 1);
		_textCharactorMode = osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT;
	}
}

void  DrawVector::createFeatureNoteText(QMap<int, QString> featurefieldmap, QMap<int, osg::Vec3> featuregeomMap, osg::ref_ptr<osg::Group> labelGroup)
{
  QList<int>  featureIDs = featurefieldmap.keys();

	for (QList<int>::iterator i = featureIDs.begin(); i != featureIDs.end(); ++i)
	{
    osg::ref_ptr<osg::LOD>    labelLod  = new osg::LOD;
    QString                   sthvalue  = featurefieldmap.value((*(i)));// feature某一字段的属性值
    osg::Vec3                 posmidle  = featuregeomMap.value((*i));
    osg::ref_ptr<osg::Geode>  textGeode = new osg::Geode;
    auto                      str       = sthvalue.toStdString();
    textGeode->addDrawable(createTextGeode(str, posmidle));

		labelLod->addChild(textGeode, 0, _highestVisibleHeight);

		labelGroup->addChild(labelLod);
	}

	QTextCodec *codec = QTextCodec::codecForName("GB2312");
  QString     ss    = codec->toUnicode(labelGroup->getName().c_str());

	recordNode(labelGroup, ss);
}

osg::ref_ptr<osgText::Text>  DrawVector::createTextGeode(std::string &txt, osg::Vec3 position)
{
  osg::ref_ptr<osgText::Text>  text = new osgText::Text;

	text->setFont("Resources/simhei.ttf");

  text->setFontResolution(35, 35);// font height and width

	text->setCharacterSize(_vecFontSize);

	text->setColor(_textColor);

	text->setLineSpacing(0.1);

	text->setAxisAlignment(osgText::TextBase::SCREEN);

	text->setCharacterSizeMode((osgText::Text::CharacterSizeMode)_textCharactorMode);

	text->setAlignment(osgText::TextBase::CENTER_CENTER);

	position.z() += 10.0;

	text->setPosition(position);

	QTextCodec *codec = QTextCodec::codecForName("GBK");
  QString     ss    = codec->toUnicode(txt.c_str());

	if (ss.contains(""))
	{
    QString  tmpss = ss.section("", 0, 0) + "㘰" + ss.section("", 1, 1);
		ss = tmpss;
	}

  text->setText(ss.toStdString(), osgText::String::ENCODING_UTF8);

	return text.release();
}
