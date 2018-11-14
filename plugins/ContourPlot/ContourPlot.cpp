#include "ContourPlot.h"

#include <iostream>
using namespace std;

#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QAction>
#include <QMenu>
#include <QToolBar>

#include <osg/LineWidth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Notify>
#include <osg/PositionAttitudeTransform>
#include <osgSim/OverlayNode>

#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

#include <DataManager/DataManager.h>

ContourPlot::ContourPlot()
{
	_pluginName = tr("Contour Plot");
	_pluginCategory = "Edit";

	endDrawing();
	_drawRoot->addChild(_pluginRoot);
	_diagonalLineVertex = new osg::Vec3Array;
}

ContourPlot::~ContourPlot(void)
{}

void ContourPlot::setupUi(QToolBar * toolBar, QMenu * menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("contourPlotAction"));
	_action->setCheckable(true);
	QIcon icon17;
	icon17.addFile(QStringLiteral("resources/icons/contour.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon17);
	_action->setVisible(true);
	_action->setText(tr("Contour"));
	_action->setToolTip(tr("Contour Analysis"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void ContourPlot::onLeftButton()
{
	if (_isDrawing == false)
	{
		beginDrawing();
		_startVertex = _anchoredWorldPos;
		_startVertexCWP = _currentWorldPos;
		
        // Init image range
		initLineGeode();
		_diagonalLineGeode = new osg::Geode;
		_diagonalLineGeode->addDrawable(_diagonalLineGeometry);
		_pluginRoot->addChild(_diagonalLineGeode);

        // Draw diagonal line
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
		_isAnalysis = true;
	}
}

void ContourPlot::onReleaseButton()
{
	if (_isAnalysis)
	{
		contourPlotFunc();
		_isAnalysis = false;
	}

}
void ContourPlot::onMouseMove()
{
	if (_isDrawing == true)
	{
		// Update the last pushed point
		_diagonalLineVertex->pop_back();
		_diagonalLineVertex->push_back(_anchoredWorldPos);
		_diagonalLineGeometry->setVertexArray(_diagonalLineVertex);
		_diagonalLineGeometry->dirtyDisplayList();
	}
}

void ContourPlot::onRightButton()
{
	if (_isDrawing == true)
	{
		_currentAnchor->removeChild(_diagonalLineGeode);
		_diagonalLineVertex->clear();
		endDrawing();
	}
}

void ContourPlot::contourPlotFunc()
{
	QString shpPath_QStr = QFileDialog::getSaveFileName(0, tr("Choose save location"), " ", "Shapefiles (*.shp);;Allfile(*.*)");
	_shpPath = shpPath_QStr;
	if (_shpPath.isEmpty())
		return;

	_demPath = (shpPath_QStr.left(shpPath_QStr.length() - 3)).append("tif");

	QString contourInterval_QStr = QInputDialog::getText(0, tr("Please enter contour interval"), "");
	if (contourInterval_QStr.isEmpty())
		return;
	else
		_dfContourInterval = contourInterval_QStr.toDouble();

	makeDEMArray(_startVertexCWP, _endVertexCWP);

	if (!_isStopped)
	{
		makeShpFile();

		//_dataManager->addElevationLayer(QString::fromStdString(_demPath), LOCAL_FILE);

		//_dataManager->addContourShpLayer(_shpPath, CONTOUR_FILE);

		//_dataManager->addElevationLayer(QString::fromStdString("C:/Users/xxfy/Desktop/drapetest/drapetest.tif"), LOCAL_FILE);

		//_dataManager->addContourShpLayer(QString::fromStdString("C:/Users/xxfy/Desktop/drapetest/drapetest.shp"), LOCAL_FILE);
	}
}
void ContourPlot::initLineGeode()
{
	_diagonalLineGeometry = new osg::Geometry;

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(_style.lineColor);
	_diagonalLineGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);

	_diagonalLineGeometry->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
}

bool ContourPlot::makeDEMArray(osg::Vec3 a, osg::Vec3 b)
{
	_minVertex.x() = a.x()<b.x() ? a.x():b.x();
	_minVertex.y() = a.y()<b.y() ? a.y():b.y();
	_maxVertex.x() = a.x()>b.x() ? a.x():b.x();
	_maxVertex.y() = a.y()>b.y() ? a.y():b.y();
	_nDemWidth = _maxVertex.x() - _minVertex.x();
	_nDemHeight = _maxVertex.y() - _minVertex.y();

	if (_nDemWidth == 0 || _nDemHeight == 0)
	{
		QMessageBox msg(QMessageBox::Warning, tr("Error"), tr("The scale is too small!"), QMessageBox::Ok);
		msg.setWindowModality(Qt::WindowModal);
		msg.exec();
		_isStopped = true;
		return false;
	}

	_demArray = new float[_nDemWidth * _nDemHeight]();
	
	QProgressDialog process; 
	process.setLabelText(tr("processing...")); 
	process.setRange(0, _nDemHeight); 
	process.setModal(true);
	process.setCancelButtonText(tr("cancel"));
	process.show();

	for (int i = 0; i < _nDemHeight; i++)
	{
		for (int j = 0; j < _nDemWidth; j++)
		{
			osgUtil::LineSegmentIntersector::Intersections intersections;  
			osg::ref_ptr<osgUtil::LineSegmentIntersector> ls = new osgUtil::LineSegmentIntersector
				(osg::Vec3(_minVertex.x() + j, _maxVertex.y() - i, 1000), 
				osg::Vec3(_minVertex.x() + j, _maxVertex.y() - i, -300));  
			osg::ref_ptr<osgUtil::IntersectionVisitor> iv = new osgUtil::IntersectionVisitor(ls);  
			_overlayNode->accept(*iv);

			if(ls->containsIntersections())  
			{  
				intersections = ls->getIntersections();  
				for(osgUtil::LineSegmentIntersector::Intersections::iterator iter = intersections.begin();iter != intersections.end();iter++)  
				{  
					_first_intersection = iter->getWorldIntersectPoint();
					break;
				}
			}

			_nIdx = i*_nDemWidth + j;
			float z_of_nIdx = _first_intersection.z();
			_demArray[_nIdx] = z_of_nIdx;
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
	poDstDS = poDriver->Create(_demPath.toStdString().c_str(), _nDemWidth, _nDemHeight,
		1, GDT_Float32, papszOptions);

	poDstDS->RasterIO(GF_Write, 0, 0, _nDemWidth, _nDemHeight,
		_demArray, _nDemWidth, _nDemHeight, GDT_Float32, 1, nullptr, 0, 0, 0);

	double topLeft[2] = { _minVertex.x(), _maxVertex.y() };

	_wkt = _globalWKT;
	poDstDS->SetProjection(_wkt);

	double adfGeoTransform[6] = { topLeft[0], 1, 0, topLeft[1], 0, -1 };
	poDstDS->SetGeoTransform(adfGeoTransform);

	GDALClose((GDALDatasetH)poDstDS);

	_isStopped = false;
	return true;
}

bool ContourPlot::makeShpFile()
{
	float fNoData = 0;

	// Read raster band
	GDALDataset * pInDataset = (GDALDataset *)GDALOpen(_demPath.toStdString().c_str(), GA_ReadOnly);
	GDALRasterBand *pInRasterBand = pInDataset->GetRasterBand(1);
	//float *pData = new float[_nDemWidth*_nDemHeight]();
	CPLErr err = pInRasterBand->RasterIO(GF_Read, 0, 0, _nDemWidth, _nDemHeight, _demArray, _nDemWidth, _nDemHeight, GDT_Float32, 0, 0);

	const char *pszDriverName = "ESRI Shapefile";
	GDALDriver *poDriver;
	GDALAllRegister();
	poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
	if (poDriver == NULL)
	{
		printf("%s driver not available.\n", pszDriverName);
		exit(1);
	}

    // Init gdal dataset
	GDALDataset *poDS;
	poDS = poDriver->Create(_shpPath.toStdString().c_str(), 0, 0, 0, GDT_Unknown, NULL);
	if (poDS == NULL)
	{
		printf("Creation of output file failed.\n");
		exit(1);
	}

    // Init gdal layer
	OGRLayer *poLayer;
	OGRSpatialReference *poSpatialRef = new OGRSpatialReference(_wkt);

	poLayer = poDS->CreateLayer("Elevation", poSpatialRef, wkbLineString, NULL);
	if (poLayer == NULL)
	{
		printf("Layer creation failed.\n");
		exit(1);
	}

    // Inserte elevation key
	OGRFieldDefn ofieldDef1("Elevation", OFTInteger);
	if (poLayer->CreateField(&ofieldDef1) != OGRERR_NONE)
	{
		osg::notify(osg::FATAL) << "Creating shape file elevation attribute table failed" << endl;
		GDALClose((GDALDatasetH)poDS);
		return false;
	}

	// Generate contour
	if (fNoData == 0)
		GDALContourGenerate(pInRasterBand, _dfContourInterval, 0, 0, NULL, false, 0, (OGRLayerH)poLayer, -1, 0, NULL, NULL);
	else
		GDALContourGenerate(pInRasterBand, _dfContourInterval, 0, 0, NULL, true, fNoData, (OGRLayerH)poLayer, -1, 0, NULL, NULL);

	poDS->SetProjection(_wkt);
	GDALClose(poDS);


	if (_demArray != NULL)
	{
		delete[] _demArray;
		_demArray = NULL;
		return false;
	}

	return true;
}