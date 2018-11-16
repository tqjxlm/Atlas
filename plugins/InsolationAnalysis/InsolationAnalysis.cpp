#include "InsolationAnalysis.h"

#include <iostream>
#include <string>
using namespace std;

#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QProgressDialog>
#include <QTime>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTreeWidgetItem>

#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/LineWidth>
#include <osgSim/OverlayNode>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>

#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

#include <DataManager/DataManager.h>
#include <DataManager/DataRecord.h>
#include <ViewerWidget/ViewerWidget.h>
#include <TileSelect/TileSelectDialog.h>

#include <DataManager/FindNode.hpp>
#include "TimeIntervalDlg.h"
#include "InsolationVisitor.h"
#include "WaitingDialog.h"

// Error limit
static const double EP = 1E-4;

// Earth radius
static const double R = 3000.0;

static osgEarth::SpatialReference* srs_wgs84 = osgEarth::SpatialReference::get("wgs84");

InsolationAnalysis::InsolationAnalysis()
{
    _pluginName = tr("Insolation Analysis");
    _pluginCategory = "Analysis";

    endDrawing();
    _drawRoot->addChild(_pluginRoot);
    _diagonalLineVertex = new osg::Vec3Array;
    connect(this, SIGNAL(goOnSA()), this, SLOT(finished()));

    connect(this, SIGNAL(loadingProgress(int)), _dataManager, SIGNAL(loadingProgress(int)));
    connect(this, SIGNAL(loadingDone()), _dataManager, SIGNAL(loadingDone()));

    //connect(_ui->okButton4SA, SIGNAL(clicked()), this, SLOT(okButton4SASlot()));
    //connect(_ui->cancelButton4SA, SIGNAL(clicked()), this, SLOT(cancelButton4SASlot()));
    //_ui->okButton4SA->setVisible(false); _ui->cancelButton4SA->setVisible(false);
}

InsolationAnalysis::~InsolationAnalysis()
{
}

void InsolationAnalysis::setupUi(QToolBar * toolBar, QMenu * menu)
{
    _action = new QAction(_mainWindow);
    _action->setObjectName(QStringLiteral("insolationAnalysisAction"));
    _action->setCheckable(true);
    QIcon icon31;
    icon31.addFile(QStringLiteral("resources/icons/sunlight.png"), QSize(), QIcon::Normal, QIcon::Off);
    _action->setIcon(icon31);
    _action->setText(tr("Insolation"));
    _action->setToolTip(tr("Insolation Analysis"));

    connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
    registerMutexAction(_action);
}

void InsolationAnalysis::onLeftButton()
{
    if (_isDrawing == false)
    {
        beginDrawing();
        _startVertex = _anchoredWorldPos;
        _startVertexCWP = _currentWorldPos;

        initLineGeode();
        _diagonalLineGeode = new osg::Geode;
        _diagonalLineGeode->addDrawable(_diagonalLineGeometry);
        _pluginRoot->addChild(_diagonalLineGeode);

        _diagonalLineVertex->push_back(_startVertex);
        _diagonalLineVertex->push_back(_anchoredWorldPos);
        _diagonalLineGeometry->setVertexArray(_diagonalLineVertex);

        _diagonalLineGeometry->removePrimitiveSet(0);
        _diagonalLineGeometry->addPrimitiveSet(new osg::DrawArrays(osg::DrawArrays::LINES, 0, _diagonalLineVertex->size()));
    }
    else
    {
        endDrawing();
        _endVertex = _anchoredWorldPos;
        _endVertexCWP = _currentWorldPos;
        _currentAnchor->removeChild(_diagonalLineGeode);
        _diagonalLineVertex->clear();

        QString shpPath_QStr = QFileDialog::getSaveFileName(0, tr("Choose save location"), " ", "Shapefiles (*.shp);;Allfile(*.*)");
        _shpPath = shpPath_QStr;
        if (_shpPath.isEmpty())
            return;
        _SAPath = ((shpPath_QStr.left(shpPath_QStr.length() - 3)).append("tif"));

        _timeIntervalDlg = new TimeIntervalDlg;
        _timeIntervalDlg->exec();
        if (_timeIntervalDlg->getStartTimeString() == "" || _timeIntervalDlg->getEndTimeString() == "")
            return;
        else
        {
            _startTime = _timeIntervalDlg->getStartTimeString();
            _endTime = _timeIntervalDlg->getEndTimeString();
        }

        _SAInterval = 60;

        makeSAArray(_startVertexCWP, _endVertexCWP);

        if (!_isStopped)
        {
            makeShpFile();
        }
    }
}

void InsolationAnalysis::onMouseMove()
{
    if (_isDrawing == true)
    {
        _diagonalLineVertex->pop_back();
        _diagonalLineVertex->push_back(_anchoredWorldPos);
        _diagonalLineGeometry->setVertexArray(_diagonalLineVertex);
        _diagonalLineGeometry->dirtyDisplayList();
    }
}

void InsolationAnalysis::onRightButton()
{
    if (_isDrawing == true)
    {
        _currentAnchor->removeChild(_diagonalLineGeode);
        _diagonalLineVertex->clear();
        endDrawing();
    }
}

void InsolationAnalysis::loadContextMenu(QMenu * contextMenu, QTreeWidgetItem * selectedItem)
{
    //if (selectedItem->parent()->text(0) == tr("Oblique Imagery Model"))
    //{
    //    auto dataRecord = dynamic_cast<DataRecord*>(selectedItem);
    //    if (dataRecord && !dataRecord->isLayer() && dataRecord->node())
    //    {
    //        contextMenu->addAction(_action);
    //        _selectedNode = dataRecord->node();
    //    }
    //}
}

void InsolationAnalysis::initLineGeode()
{
    _diagonalLineGeometry = new osg::Geometry;

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(_style.lineColor);
    _diagonalLineGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);

    _diagonalLineGeometry->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
}

bool InsolationAnalysis::makeSAArray(osg::Vec3 a, osg::Vec3 b)
{
    _minVertex.x() = a.x() < b.x() ? a.x() : b.x();
    _minVertex.y() = a.y() < b.y() ? a.y() : b.y();
    _maxVertex.x() = a.x() > b.x() ? a.x() : b.x();
    _maxVertex.y() = a.y() > b.y() ? a.y() : b.y();
    _nArrayWidth = _maxVertex.x() - _minVertex.x();
    _nArrayHeight = _maxVertex.y() - _minVertex.y();
    _SAArray = new float[_nArrayWidth * _nArrayHeight]();

    QProgressDialog process;
    process.setLabelText(tr("processing..."));
    process.setRange(0, _nArrayHeight);
    process.setModal(true);
    process.setCancelButtonText(tr("cancel"));
    process.show();

    for (int i = 0; i < _nArrayHeight; i++)
    {
        for (int j = 0; j < _nArrayWidth; j++)
        {
            osgUtil::LineSegmentIntersector::Intersections intersections;
            osg::ref_ptr<osgUtil::LineSegmentIntersector> ls = new osgUtil::LineSegmentIntersector
            (osg::Vec3(_minVertex.x() + j, _maxVertex.y() - i, 1000),
                osg::Vec3(_minVertex.x() + j, _maxVertex.y() - i, -300));
            osg::ref_ptr<osgUtil::IntersectionVisitor> iv = new osgUtil::IntersectionVisitor(ls);
            _overlayNode->accept(*iv);

            if (ls->containsIntersections())
            {
                intersections = ls->getIntersections();
                for (osgUtil::LineSegmentIntersector::Intersections::iterator iter = intersections.begin(); iter != intersections.end(); iter++)
                {
                    _first_intersection = iter->getWorldIntersectPoint();
                    _pointA = _first_intersection;
                    _geo_pointA = worldPos2GeoPos(_pointA);
                    break;
                }
            }

            _nIdx = i*_nArrayWidth + j;
            _SAArray[_nIdx] = getIllumination(_startTime, _endTime, 30, _pointA, _geo_pointA.x(), _geo_pointA.y());
        }
        process.setValue(i);
        if (process.wasCanceled())
        {
            _isStopped = true;
            process.close();
            return false;
        }
    }
    process.close();

    GDALAllRegister();
    const char *pszFormat = "GTiff";
    GDALDriver *poDriver;
    poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    char **papszOptions = NULL;
    papszOptions = CSLSetNameValue(papszOptions, "TILED", "YES");

    GDALDataset *poDstDS;
    poDstDS = poDriver->Create(_SAPath.toStdString().c_str(), _nArrayWidth, _nArrayHeight,
        1, GDT_Float32, papszOptions);

    poDstDS->RasterIO(GF_Write, 0, 0, _nArrayWidth, _nArrayHeight,
        _SAArray, _nArrayWidth, _nArrayHeight, GDT_Float32, 1, nullptr, 0, 0, 0);

    double topLeft[2] = { _minVertex.x(), _maxVertex.y() };

    poDstDS->SetProjection(_globalWKT);

    double adfGeoTransform[6] = { topLeft[0], 1, 0, topLeft[1], 0, -1 };
    poDstDS->SetGeoTransform(adfGeoTransform);

    GDALClose((GDALDatasetH)poDstDS);

    _isStopped = false;
    return true;
}

bool InsolationAnalysis::makeShpFile()
{
    float fNoData = 0;

    GDALDataset * pInDataset = (GDALDataset *)GDALOpen(_SAPath.toStdString().c_str(), GA_ReadOnly);
    GDALRasterBand *pInRasterBand = pInDataset->GetRasterBand(1);
    CPLErr err = pInRasterBand->RasterIO(GF_Read, 0, 0, _nArrayWidth, _nArrayHeight, _SAArray, _nArrayWidth, _nArrayHeight, GDT_Float32, 0, 0);

    const char *pszDriverName = "ESRI Shapefile";
    GDALDriver *poDriver;
    GDALAllRegister();
    poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
    if (poDriver == NULL)
    {
        osg::notify(osg::FATAL) << pszDriverName << " driver not available.\n";
        exit(1);
    }

    GDALDataset *poDS;
    poDS = poDriver->Create(_shpPath.toStdString().c_str(), 0, 0, 0, GDT_Unknown, NULL); //创建数据源
    if (poDS == NULL)
    {
        osg::notify(osg::FATAL) << "Creation of output file failed.\n";
        exit(1);
    }

    OGRLayer *poLayer;
    OGRSpatialReference *poSpatialRef = new OGRSpatialReference(_globalWKT);

    poLayer = poDS->CreateLayer("Elevation", poSpatialRef, wkbLineString, NULL);
    if (poLayer == NULL)
    {
        osg::notify(osg::FATAL) << "Layer creation failed.\n";
        exit(1);
    }

    OGRFieldDefn ofieldDef1("Elevation", OFTInteger);
    if (poLayer->CreateField(&ofieldDef1) != OGRERR_NONE)
    {
        osg::notify(osg::FATAL) << "Failed to create vector layer attribution！" << endl;
        GDALClose((GDALDatasetH)poDS);
        return false;
    }

    if (fNoData == 0)
        GDALContourGenerate(pInRasterBand, _SAInterval, 0, 0, NULL, false, 0, (OGRLayerH)poLayer, -1, 0, NULL, NULL);
    else
        GDALContourGenerate(pInRasterBand, _SAInterval, 0, 0, NULL, true, fNoData, (OGRLayerH)poLayer, -1, 0, NULL, NULL);

    poDS->SetProjection(_globalWKT);
    GDALClose(poDS);


    if (_SAArray != NULL)
    {
        delete[] _SAArray;
        _SAArray = NULL;
        return false;
    }

    return true;
}

osg::Vec3 InsolationAnalysis::worldPos2GeoPos(osg::Vec3 worldPos)
{
    osgEarth::GeoPoint currentGeoPos = osgEarth::GeoPoint(
        _globalSRS, worldPos.x(), worldPos.y(), worldPos.z()).transform(srs_wgs84);
    osg::Vec3 geoPos;
    geoPos.x() = currentGeoPos.x();
    geoPos.y() = currentGeoPos.y();
    geoPos.z() = currentGeoPos.z();
    return geoPos;
}

void InsolationAnalysis::okButton4SASlot()
{
}

void InsolationAnalysis::cancelButton4SASlot()
{
}

void InsolationAnalysis::toggle(bool checked)
{
    if (checked)
    {
        if (!_selectedNode)
        {
            QMessageBox::warning(_mainWindow, tr("Notice:"), tr("Please select a model first."), QMessageBox::Ok);
            _action->toggle();
            return;
        }
        toggleTileSelectDialog(_selectedNode, true);

    }
    else
    {
        _root->removeChild(findNodeInNode("hudCamera", _root));
    }

    _activated = checked;
}

time_t InsolationAnalysis::convert_str_to_tm(const char * str_time)
{
    // Unit: second
    struct tm tt;
    memset(&tt, 0, sizeof(tt));
    tt.tm_year = atoi(str_time) - 1900;
    tt.tm_mon = atoi(str_time + 5) - 1;
    tt.tm_mday = atoi(str_time + 8);
    tt.tm_hour = atoi(str_time + 11);
    tt.tm_min = atoi(str_time + 14);
    tt.tm_sec = atoi(str_time + 17);
    return mktime(&tt);
}

osg::Vec2 InsolationAnalysis::getSolarAngle(const char* str_time1, int N, double lon, double lat)
{
    // Get time zone
    int timeZone;
    if (fmod(lon, 15) < 7.5)
        timeZone = lon / 15;
    else
        timeZone = (lon / 15) + 1;

    // Compute solar altitude and azimuth
    double N0, sitar, ED, dLon, Et;
    double dTimeAngle, gtdt, latitudeArc, HeightAngleArc;
    double AzimuthAngleArc, CosAzimuthAngle;
    int year, hour, min, sec;

    year = atoi(str_time1);
    hour = atoi(str_time1 + 11);
    min = atoi(str_time1 + 14);
    sec = atoi(str_time1 + 17);

    N0 = 79.6764 + 0.2422 * (year - 1985) - floor((year - 1985) / 4.0);
    sitar = 2 * osg::PI * (N - N0) / 365.2422;
    ED = 0.3723 + 23.2567 * sin(sitar) + 0.1149 * sin(2 * sitar) - 0.1712 * sin(3 * sitar) - 0.758 * cos(sitar) + 0.3656 * cos(2 * sitar) + 0.0201 * cos(3 * sitar);//太阳赤纬角计算公式
    ED = ED * osg::PI / 180;

    dLon = 0.0;

    // Longitude distance to the center of the timezone
    if (lon >= 0)
    {
        if (timeZone == -13)
            dLon = lon - (floor((lon * 10 - 75) / 150) + 1)*15.0;
        else
            dLon = lon - timeZone*15.0;
    }
    else
    {
        if (timeZone == -13)
            dLon = (floor((lon * 10 - 75) / 150) + 1)*15.0 - lon;
        else
            dLon = timeZone*15.0 - lon;
    }

    Et = 0.0028 - 1.9857*sin(sitar) + 9.9059*sin(2 * sitar) - 7.0924*cos(sitar) - 0.6882*cos(2 * sitar);
    gtdt = hour + min / 60.0 + sec / 3600.0 + dLon / 15;
    gtdt = gtdt + Et / 60.0;
    dTimeAngle = 15.0*(gtdt - 12);
    dTimeAngle = dTimeAngle * osg::PI / 180;
    latitudeArc = lat * osg::PI / 180;

    HeightAngleArc = asin(sin(latitudeArc)*sin(ED) + cos(latitudeArc)*cos(ED)*cos(dTimeAngle));
    CosAzimuthAngle = (sin(HeightAngleArc)*sin(latitudeArc) - sin(ED)) / cos(HeightAngleArc) / cos(latitudeArc);
    AzimuthAngleArc = acos(CosAzimuthAngle);

    double tmp_heightAngle = HeightAngleArc * 180 / osg::PI;
    double tmp_azimuthAngle = AzimuthAngleArc * 180 / osg::PI;

    if (dTimeAngle < 0)
    {
        tmp_azimuthAngle = 180 - tmp_azimuthAngle;
    }
    else
    {
        tmp_azimuthAngle = 180 + tmp_azimuthAngle;
    }

    osg::Vec2 angleValue;
    angleValue.x() = tmp_azimuthAngle;
    angleValue.y() = tmp_heightAngle;
    return angleValue;
}

osg::Vec3 InsolationAnalysis::rayEquation(osg::Vec3 pointA, double azimuthAngle, double heightAngle)
{
    double px, py, pz;
    azimuthAngle = azimuthAngle * osg::PI / 180;
    heightAngle = heightAngle * osg::PI / 180;

    px = -cos(heightAngle)*sin(azimuthAngle);
    py = cos(heightAngle)*cos(azimuthAngle);
    pz = sin(heightAngle);

    osg::Vec3 pointB;
    pointB.x() = px*R + pointA.x();
    pointB.y() = py*R + pointA.y();
    pointB.z() = pz*R + pointA.z();

    return pointB;
}

float InsolationAnalysis::getIllumination(QString start_time, QString end_time, int step, osg::Vec3 pointA, double lon, double lat)
{
    float illumination = 0;
    string str_time1 = start_time.toStdString();
    string str_time2 = end_time.toStdString();

    // Compute distance and time range
    struct tm *p;
    time_t  totalMin, secTime, cycleIndex;
    char str_time[64] = "2017-01-01 00:00:00";
    strncpy(str_time, str_time1.c_str(), 4);

    totalMin = (convert_str_to_tm(str_time2.c_str()) - convert_str_to_tm(str_time1.c_str())) / 60;
    _totalMin = totalMin;
    cycleIndex = totalMin / step;

    secTime = convert_str_to_tm(str_time1.c_str());
    int N = (convert_str_to_tm(str_time1.c_str()) - convert_str_to_tm(str_time)) / (3600 * 24);

    // For every time frame in the cycle, compute insolation rate for the scene
    for (int i = 0; i <= cycleIndex; i++)
    {
        int today = atoi(str_time1.c_str() + 8);
        int year = atoi(str_time1.c_str());

        osg::Vec2 twoAngles = getSolarAngle(str_time1.c_str(), N, lon, lat);

        // The starting time should increment along with the cycle
        secTime += (step * 60);
        p = localtime(&secTime);
        strftime(const_cast<char*>(str_time1.c_str()), 64, "%Y-%m-%d %H:%M:%S", p);

        if (today != atoi(str_time1.c_str() + 8))
            N += 1;
        if (year != atoi(str_time1.c_str()))
            N = 1;

        if (twoAngles.y() <= 0)
            continue;

        osg::Vec3 pointB = rayEquation(pointA, twoAngles.x(), twoAngles.y());

        bool light = true;

        // Use intersections to check lighting
        osgUtil::LineSegmentIntersector::Intersections intersections;
        osg::ref_ptr<osgUtil::LineSegmentIntersector> ls =
            new osgUtil::LineSegmentIntersector({ pointA.x(), pointA.y(), pointA.z() },
            { pointB.x(), pointB.y(), pointB.z() });
        osg::ref_ptr<osgUtil::IntersectionVisitor> iv = new osgUtil::IntersectionVisitor(ls);
        _overlayNode->accept(*iv);

        // Output all intersections
        if (ls->containsIntersections())
        {
            intersections = ls->getIntersections();

            // If intersected twice
            if (intersections.size() > 1)
            {
                // Not lighted
                light = false;
            }
        }

        if (light == true)
            illumination += step;
    }

    return illumination;
}

float InsolationAnalysis::getTotalMin()
{
    return _totalMin;
}

void InsolationAnalysis::finished()
{
    /*WaitingDialog*  waitDlg = new WaitingDialog(tr("Please wait..."));
    waitDlg->show();
    waitDlg->startAnimation();

    QTime t;
    t.start();
    while (t.elapsed() < 500)
    	QCoreApplication::processEvents();

    _activated = false;

    bool checked = true;

    LoadingThread loadingThread(_dataManager->_nodeGroup4SA);
    loadingThread.start();
    while (loadingThread.getFinishStatus() == false)
    	QCoreApplication::processEvents();


    InsolationVisitor visitor4Matrix;
    _dataManager->getActivatedNode()->accept(visitor4Matrix);

    delete waitDlg;

    loadingThread.done();

    TimeIntervalDlg *timeIntervalDlg = new TimeIntervalDlg;
    timeIntervalDlg->exec();
    QString startTime;
    QString endTime;
    if (timeIntervalDlg->getStartTimeString() == "" || timeIntervalDlg->getEndTimeString() == "")
    	return;
    else
    {
    	startTime = timeIntervalDlg->getStartTimeString();
    	endTime = timeIntervalDlg->getEndTimeString();
    }


    float pbValue = 0;
    emit loadingProgress(0);


    osg::ref_ptr<osg::Group> geodeGroup = new osg::Group;
    geodeGroup->setName("SunlightAnalysis");


    osg::Vec3 localVertex4SA;
    osg::Vec3 worldVertex4SA;
    osg::Vec3 geoVertex4SA;

    for (unsigned int i = 0; i < loadingThread.getFSV().getGeodeGroup()->getNumChildren(); i++)
    {

    	osg::ref_ptr<osg::Geode> tmpGeode = loadingThread.getFSV().getGeodeGroup()->getChild(i)->asGeode();


    	unsigned int drawablesCount = tmpGeode->getNumDrawables();
    	for (unsigned int i = 0; i < drawablesCount; i++)
    	{
    		osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    		osg::ref_ptr<osg::Geometry> geometry = tmpGeode->getDrawable(i)->asGeometry();
    		if (geometry)
    		{

    			osg::ref_ptr<osg::Vec3Array> vertex = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
    			for (int i = 0; i < vertex->size(); i++)
    			{
    				localVertex4SA = vertex->at(i);
    				worldVertex4SA = localVertex4SA * (visitor4Matrix.getGeodeMatrix());
    				geoVertex4SA = worldPos2GeoPos(worldVertex4SA);

    				float illumination = getIllumination(startTime, endTime, 60, worldVertex4SA, geoVertex4SA.x(), geoVertex4SA.y());
    				float degree = illumination / getTotalMin();




    				if (degree < 0.5)
    				{
    					colors->push_back(osg::Vec4(degree * 2, 1.0f, 0.0f, 1.0f));
    				}
    				else
    				{
    					colors->push_back(osg::Vec4(1.0f, (1 - degree) * 2, 0.0f, 1.0f));
    				}
    			}

    			geometry->setColorArray(colors.get());
    			geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    		}
    	}

    	pbValue = 100 * i / (loadingThread.getFSV().getGeodeGroup()->getNumChildren());
    	emit loadingProgress(pbValue);
    }


    osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
    mt->setMatrix(visitor4Matrix.getGeodeMatrix());
    mt->addChild(loadingThread.getFSV().getGeodeGroup());

    _currentAnchor->addChild(mt);

    recordNode(loadingThread.getFSV().getGeodeGroup());

    emit loadingDone();



    float _vMin = 0.0f;
    float _vMax = getTotalMin();
    float interval = (_vMax - _vMin) / 10.0f;

    QVector<osg::Vec4> colorVector;
    QVector<QString> txtVector;

    for (int i = 0; i < 10; i++)
    {

    	float nLeft = _vMin + i * interval;
    	float nRight = _vMin + (i + 1) * interval;
    	float nMiddle = (i + 1) / 10.0f;


    	osg::Vec4 color;
    	if (nMiddle < 0.5f)
    	{
    		color = osg::Vec4(nMiddle * 2.0f, 1.0f, 0.0f, 1.0f);
    	}
    	else
    	{
    		color = osg::Vec4(1.0f, (1 - nMiddle) * 2.0f, 0.0f, 1.0f);
    	}
    	colorVector.append(color);


    	char buffer[20];
    	sprintf_s(buffer, "%.0f", nLeft);
    	QString str = buffer;
    	str += " - ";
    	sprintf_s(buffer, "%.0f", nRight);
    	str += buffer;
    	str += " min";

    	txtVector.append(str);
    }

    _root->addChild(ViewerWidget::createLegendHud(tr("legent"), colorVector, txtVector));*/
}
