// #include "StdAfx.h"
#include "SaveOrthoDSMDialog.h"

#include <osg/ComputeBoundsVisitor>
#include <QFileDialog>
#include <QThread>
#include <iostream>
#include <gdal/gdal.h>

using namespace std;
const double  modelZOffset   = 0.2;
const double  captureZOffset = 100;

SaveOrthoDSMDialog::SaveOrthoDSMDialog(ViewerWidget &vw, osg::Node *scene, std::string wkt, QWidget *parent):
  QDialog(parent),
	_vw(vw),
  _view(*vw.getMainView()),
	_scene(scene),
	_srsWKT(wkt),
	_activeMode(false),
	_successed(false),
	_finished(false),
	_bgColor(1.0, 1.0, 1.0, 1.0)
{
	_ui.setupUi(this);
	connect(_ui.pushButtonOk, SIGNAL(clicked()), this, SLOT(startCapturing()));
	connect(_ui.tileNumSpinBox, SIGNAL(valueChanged(int)), _ui.tileNumSpinBox_2, SLOT(setValue(int)));
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	_ui.tileNumSpinBox->setValue(1);
	_ui.pixelSpinBox->setValue(0.2);
}

SaveOrthoDSMDialog::~SaveOrthoDSMDialog()
{
	if (!_finished)
  {
    finish();
  }
}

void  SaveOrthoDSMDialog::setup()
{
  // 显示窗口
  osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();
  unsigned int                                    screenWidth, screenHeight;
	wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
	this->move((screenWidth - this->width()) / 2, (screenHeight - this->height()) / 2);
	this->setWindowFlags(this->windowFlags() & ~Qt::CustomizeWindowHint & ~Qt::WindowMinMaxButtonsHint);
	this->setWindowModality(Qt::WindowModal);
	this->show();
}

void  SaveOrthoDSMDialog::startCapturing()
{
	_path = QFileDialog::getExistingDirectory(0, tr("Choose save location"), ".");

	if (_path.isEmpty())
  {
    return;
  }

  _waitDialog = new WaitProgressDialog("", "", 0,
                                       ((osg::MatrixTransform *)_scene)->getNumChildren(), this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	connect(this, SIGNAL(updateWaitMessage(const QString&)), _waitDialog, SLOT(updateMessage(const QString&)));
	connect(this, SIGNAL(updateTime()), _waitDialog, SLOT(updateTime()));
	_waitDialog->adjustSize();
	_waitDialog->setWindowModality(Qt::WindowModal);
	_waitDialog->show();

	this->hide();
	_view.getDatabasePager()->setDoPreCompile(false);

	// 内存充足的情况下，使用300的LOD数以提高程序效率
	_origMaxLOD = _view.getDatabasePager()->getTargetMaximumNumberOfPageLOD();
  // _view.getDatabasePager()->setTargetMaximumNumberOfPageLOD(2000);
	_vw.stopRendering();

  // 保存原视图状态
  _orthoCamera     = _view.getCamera();
  _origViewWidth   = _orthoCamera->getViewport()->width();
  _origViewHeight  = _orthoCamera->getViewport()->height();
  _origViewMatrix  = _orthoCamera->getViewMatrix();
  _origProjMatrix  = _orthoCamera->getProjectionMatrix();
	_origNearFarMode = _orthoCamera->getComputeNearFarMode();
  _origManip       = _view.getCameraManipulator();
  _origScene       = _view.getSceneData();

	// 计时
  QTimer  *timer;
  QThread *timerThread;

	timer = new QTimer;
	timer->setInterval(1000);
	connect(timer, SIGNAL(timeout()), _waitDialog, SLOT(updateTime()));
  timerThread = new QThread();
	timer->moveToThread(timerThread);
	connect(timerThread, SIGNAL(started()), timer, SLOT(start()));
	timerThread->start();

  QTime  totalTime;
	totalTime.start();

  QTime  tileTime;
	tileTime.start();

  osg::ref_ptr<osg::MatrixTransform>  subScene = new osg::MatrixTransform;
  osg::Matrix                         m;
  m.makeTranslate(((osg::MatrixTransform *)_scene)->getMatrix().getTrans() - osg::Vec3d(0, 0, -modelZOffset));
	subScene->setMatrix(m);

	// 以切片为单位进行逐个处理
  for (unsigned int i = 0; i < ((osg::MatrixTransform *)_scene)->getNumChildren(); i++)
	{
		subScene->removeChildren(0, subScene->getNumChildren());
    _subTile = ((osg::MatrixTransform *)_scene)->getChild(i);
		subScene->addChild(_subTile);

    QString  tileName = QString::fromStdString(_subTile->getName()).split('/').back().split('.').front();
    QString  saveName = _path + "/" + tileName + ".tiff";
    _fileList.push_back(tileName + ".tiff");

		advanceCapturing(tr("Processing ") + tileName);

    // if (tileName != "Tile_+1002_+1016")
    // {
    // cout << "Tile not wanted, skipped: " << i << ", " << tileName.toStdString() << endl;
    // continue;
    // }
		if (!_ui.overwriteCheckBox->isChecked() && QFileInfo(saveName).exists())
		{
			cout << "Tile exist, skipped: " << i << ", " << tileName.toStdString() << endl;
			continue;
		}

		cout << "Tile export started: " << i << ", " << tileName.toStdString() << endl;

    osg::ComputeBoundsVisitor  boundsVisitor;
		boundsVisitor.apply(*subScene);
    _boundingBox    = boundsVisitor.getBoundingBox();
    _boundingWidth  = _boundingBox.xMax() - _boundingBox.xMin() + 1;
    _boundingHeight = _boundingBox.yMax() - _boundingBox.yMin() + 1;
    unsigned int  viewWidth  = _boundingWidth + 1;
    unsigned int  viewHeight = _boundingHeight + 1;

    // 计算视口大小
    unsigned int  widgetWidth, widgetHeight;
    widgetWidth  = _vw.width();
		widgetHeight = _vw.height();

		if (viewWidth >= widgetWidth - 50)
		{
			viewHeight = (double)viewHeight / viewWidth * (widgetWidth - 50);
      viewWidth  = widgetWidth - 50;
		}

		if (viewHeight >= widgetHeight - 50)
		{
      viewWidth  = (double)viewWidth / viewHeight * (widgetHeight - 50);
			viewHeight = widgetHeight - 50;
		}

    // 设置视图和投影
    osg::Vec3  eye, center, up;
    eye     = _boundingBox.center();
		eye.z() = _boundingBox.zMax() + captureZOffset;
    center  = eye - osg::Vec3(0, 0, 1);
    up      = osg::Vec3(0, 1, 0);
		_orthoCamera->setViewport((widgetWidth - 50 - viewWidth) / 2, 25, viewWidth, viewHeight);
    _orthoCamera->setViewMatrixAsLookAt(eye, center, up);
		_orthoCamera->setProjectionMatrixAsOrtho(-_boundingWidth / 2, _boundingWidth / 2,
                                             -_boundingHeight / 2, _boundingHeight / 2, 1, 10000);
		_orthoCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // 初始化捕捉节点
		_captureRoot = new osg::Switch;
		_captureRoot->addChild(subScene, true);
		_view.setSceneData(_captureRoot);
		_view.setCameraManipulator(0);

    // 输出设置
    _numTiles      = _ui.tileNumSpinBox->value();
    _pixelPerMeter = _ui.pixelSpinBox->value();
    _pixelPerMeter = 1 / _pixelPerMeter;// 改为像素分辨率by jt
    _tileWidth     = _boundingWidth / _numTiles * _pixelPerMeter + 1;
    _tileHeight    = _boundingHeight / _numTiles * _pixelPerMeter + 1;
    _posterWidth   = _tileWidth * _numTiles;
    _posterHeight  = _tileHeight * _numTiles;

    // 设置离屏渲染相机
    osg::ref_ptr<osg::Camera>  captureCamera = new osg::Camera;
		captureCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		captureCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
		captureCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    // captureCamera->setClearDepth(0);
    osg::Camera::RenderTargetImplementation  renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT;
		captureCamera->setRenderTargetImplementation(renderImplementation);
		captureCamera->setViewport(0, 0, _tileWidth, _tileHeight);
		captureCamera->addChild(subScene);
		captureCamera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
		_captureRoot->addChild(captureCamera, true);

    // 初始化输出
		_printer = new PosterPrinter;
		_printer->setTileSize(_tileWidth, _tileHeight);
		_printer->setPosterSize(_posterWidth, _posterHeight);
		_printer->setOutputType(PosterPrinter::DEPTH);
		_printer->setCamera(captureCamera);
		_printer->setOutputTiles(false);
		_printer->setOutputTileExtension("tiff");
		_printer->setSRS(_srsWKT);
		_printer->setPixel(_pixelPerMeter);

    // 初始化图像
    osg::ref_ptr<osg::Image>  posterImage = 0;
		posterImage = new osg::Image;
		posterImage->allocateImage(_posterWidth, _posterHeight, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
		_printer->setFinalPoster(posterImage.get());
		_printer->setOutputPosterName(_path.toLocal8Bit().toStdString());
		_printer->setPath(_path);
		_printer->setViewCamera(_orthoCamera);

    // 开始捕捉
    // waitForCapturing(true);

		_printer->init(_view.getCamera());

    osg::Camera                  *camera   = _view.getCamera();
    osg::ref_ptr<CustomRenderer>  renderer = new CustomRenderer(camera);
		camera->setRenderer(renderer.get());

		while (!_printer->done())
		{
			_vw.advance();

			// Keep updating and culling until full level of detail is reached
			renderer->setCullOnly(true);

			while (_view.getDatabasePager()->getRequestsInProgress())
			{
				_vw.updateTraversal();
				_vw.renderingTraversals();
        // Sleep(1);
			}

			renderer->setCullOnly(false);
			_printer->frame(_view.getFrameStamp(), _view.getSceneData());
			_vw.renderingTraversals();
    }

    // waitForCapturing(false);
		writeWithGDAL();
		cout << "Finished, time consumed: " << tileTime.restart() / 1000 << endl;
		cout << "Total time consumed: " << totalTime.elapsed() / 1000 << endl;

    // osg::PagedLOD* tile = ((osg::PagedLOD*)_subTile.get());

    // for (int i = 0; i < tile->getNumChildren(); i++)
    // {
    // tile->setMinimumExpiryFrames(i, 1);
    // }
	}

	_successed = true;
	timerThread->terminate();
	timer->stop();
	delete timer;
	delete timerThread;
  emit  accepted();
}

void  SaveOrthoDSMDialog::finish()
{
	if (_successed)
	{
		// 镶嵌图片
		if (_ui.mergeCheckBox->isChecked())
		{
      WaitProgressDialog *waitDialog = new WaitProgressDialog(tr("Merging tiles..."), "", 0, 1, 0, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
			waitDialog->adjustSize();
			waitDialog->setWindowModality(Qt::WindowModal);

			// 使用gdalbuildvrt工具进行镶嵌，生成vrt文件
      QStringList  commandList(_fileList);
			commandList.push_front("output.vrt");
			commandList.push_front("-overwrite");
			commandList.push_front(_srsWKT.c_str());
			commandList.push_front("-a_srs");

      QStringList  environment;
			environment.push_back("GDAL_DATA=./GDAL/data");

			QProcess *process = new QProcess(this);
			process->setWorkingDirectory(_path);
			cout << "cd " << _path.toStdString() << endl;
			cout << "gdalbuildvrt " << commandList.join(' ').toStdString() << endl;
			process->setStandardOutputFile(_path + "/mosaicLog.txt");
			process->setStandardErrorFile(_path + "/mosaicError.txt");
			process->setEnvironment(environment);

			// 镶嵌结束，转换格式
      connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
              [ = ](int exitCode, QProcess::ExitStatus exitStatus)
      {
				// 使用gdal_translate工具转换成tiff文件
				QStringList commandList;
				commandList.push_front("output.tiff");
				commandList.push_front("output.vrt");
				commandList.push_front(_srsWKT.c_str());
				commandList.push_front("-a_srs");
				commandList.push_front("GTiff");
				commandList.push_front("-of");

				QProcess *process = new QProcess(this);
				process->setWorkingDirectory(_path);
				cout << "gdal_translate " << commandList.join(' ').toStdString() << endl;
				process->setStandardOutputFile(_path + "/translateLog.txt");
				process->setStandardErrorFile(_path + "/translateError.txt");
				process->setEnvironment(environment);

				// 转换结束，处理噪点
        connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                [ = ](int exitCode, QProcess::ExitStatus exitStatus)
        {
					cout << "Fixing nodata values" << endl;

					GDALDataset  *poDataset;
					GDALAllRegister();
					poDataset = (GDALDataset *)GDALOpen((_path + "/output.tiff").toLocal8Bit().toStdString().c_str(), GA_Update);

          // 差值处理坏点
					CPLErr success = GDALFillNodata(poDataset->GetRasterBand(1), 0, 5, 0, 0, NULL, 0, 0);

					if (success == CE_Failure)
					{
            cout << "Nodata fix failed" << endl;
					}

					GDALClose((GDALDatasetH)poDataset);

					waitDialog->close();
				});
				process->start("gdal_translate.exe", commandList);
			});
			process->start("gdalbuildvrt.exe", commandList);
			waitDialog->exec();
		}

		QMessageBox::information(this, tr("Exporting completed"), tr("Exported to ") + _path, QMessageBox::NoButton);
		_waitDialog->close();

		// 还原视图原始状态
		_vw.startRendering();

		_view.getDatabasePager()->setTargetMaximumNumberOfPageLOD(_origMaxLOD);
		_view.setCameraManipulator(_origManip);
		_view.setSceneData(_origScene);

		_orthoCamera->setViewport(0, 0, _origViewWidth, _origViewHeight);
		_orthoCamera->setProjectionMatrix(_origProjMatrix);
		_orthoCamera->setViewMatrix(_origViewMatrix);
		_orthoCamera->setComputeNearFarMode(_origNearFarMode);

		_captureRoot = 0;
    _printer     = 0;
	}

	_finished = true;
}

void  SaveOrthoDSMDialog::captureFailed(const QString& msg)
{
  QMessageBox::critical(this, tr("Error"), tr("Insufficient memory. Please lower pixel number or increase tile number and retry."));
	_waitDialog->close();
	delete _waitDialog;
	this->show();
}

void  SaveOrthoDSMDialog::fixNodata(int exitCode, QProcess::ExitStatus exitStatus)
{
  GDALDataset *poDataset;

	GDALAllRegister();
	poDataset = (GDALDataset *)GDALOpen((_path + "/output.tiff").toLocal8Bit().toStdString().c_str(), GA_Update);

  // 差值处理坏点
  CPLErr  success = GDALFillNodata(poDataset->GetRasterBand(1), 0, 5, 0, 0, NULL, 0, 0);

	if (success == CE_Failure)
	{
		cout << "Nodata fix failed" << endl;
	}

	GDALClose((GDALDatasetH)poDataset);

	_waitDialog->close();
}

void  SaveOrthoDSMDialog::waitForCapturing(bool started)
{
  static QTimer  *timer;
  static QThread *timerThread;

	if (started)
	{
	}
	else if (!_successed)
	{
    // _waitDialog->close();
    // _successed = true;
    // emit accepted();
	}
}

void  SaveOrthoDSMDialog::advanceCapturing(const QString& msg)
{
	if (!_successed)
	{
		_msg = msg;
		_waitDialog->setValue(_waitDialog->value() + 1);
    emit  updateWaitMessage(msg);
	}
}

void  SaveOrthoDSMDialog::writeWithGDAL()
{
	GDALAllRegister();
	// Default to GTiff driver, other drivers are also possible
	const char *pszFormat = "GTiff";
	GDALDriver *poDriver;
  // char **papszMetadata;
	poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

	// TODO: Only use TILED at present, other opetions like compression, preview or multithread should also be available
	// More details: http://www.gdal.org/frmt_gtiff.html
	char **papszOptions = NULL;
  // papszOptions = CSLSetNameValue(papszOptions, "TILED", "YES");
  // papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "JPEG" );

  std::string  fileName = QString::fromStdString(_subTile->getName()).split('/').back().split('.').front().toStdString();
  std::string  saveName = _path.toLocal8Bit().toStdString() + "/" + fileName + ".tiff";

	// Create the file and copy raster data
	GDALDataset *poDstDS;
	poDstDS = poDriver->Create(saveName.c_str(), _posterWidth, _posterHeight,
                             1, GDT_Float32, papszOptions);
	poDstDS->RasterIO(GF_Write, 0, 0, _posterWidth, _posterHeight,
                    (float *)(_printer->getFinalPoster()->data()) + _posterWidth * (_posterHeight - 1),
                    _posterWidth, _posterHeight, GDT_Float32, 1, nullptr,
                    sizeof(float), -_posterWidth * sizeof(float), _posterWidth * _posterHeight * sizeof(float));
	poDstDS->GetRasterBand(1)->SetNoDataValue(10000);

	// Origin default to top left
  double  topLeft[2] = { _boundingBox.corner(2)[0], _boundingBox.corner(2)[1] };

	poDstDS->SetProjection(_srsWKT.c_str());

	// Affine information
  double  adfGeoTransform[6] = { topLeft[0], 1 / _pixelPerMeter, 0, topLeft[1], 0, -1 / _pixelPerMeter };
  // GDALRasterBand *poBand;
	poDstDS->SetGeoTransform(adfGeoTransform);

	GDALClose((GDALDatasetH)poDstDS);
}

// void SaveOrthoDSMDialog::writeWithGDAL()
// {
// GDALAllRegister();
//// Default to GTiff driver, other drivers are also possible
// const char *pszFormat = "GTiff";
// GDALDriver *poDriver;
// char **papszMetadata;
// poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
//
//// TODO: Only use TILED at present, other opetions like compression, preview or multithread should also be available
//// More details: http://www.gdal.org/frmt_gtiff.html
// char **papszOptions = NULL;
////papszOptions = CSLSetNameValue( papszOptions, "TILED", "YES" );
////papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "JPEG" );
//
//// Create the file and copy raster data
// GDALDataset *poDstDS;
// std::string fileName = QString::fromStdString(_subTile->getName()).split('/').back().split('.').front().toStdString();
// std::string saveName = "D:/testImage/" + fileName + ".tiff";
//
// poDstDS = poDriver->Create(saveName.c_str(), _posterWidth, _posterHeight,
// 3, GDT_Byte, papszOptions );
//
// poDstDS->RasterIO(GF_Write, 0, 0, _posterWidth, _posterHeight,
// (unsigned char*)(_printer->getFinalPoster()->data()) + _posterWidth * (_posterHeight - 1) * 3,
// _posterWidth, _posterHeight, GDT_Byte, 3, nullptr, 3, -_posterWidth * 3, 1);
//
//
// for (int i = 1; i <= 3; i++)
// {
// poDstDS->GetRasterBand(i)->SetNoDataValue(0);
// }
//
//// Origin default to top left
// double topLeft[2] = {_boundingBox.corner(2)[0], _boundingBox.corner(2)[1]};
//
//// ---------------------------投影方式的选择-----------------------------
//
//// 1. WGS_1984_Web_Mercator投影
////char* wgs84_merc = "PROJCS[\"WGS_1984_Web_Mercator\",\
/// // //	//	GEOGCS[\"GCS_WGS_1984_Major_Auxiliary_Sphere\",\
/// // //	//	DATUM[\"D_WGS_1984_Major_Auxiliary_Sphere\",\
/// // //	//	SPHEROID[\"WGS_1984_Major_Auxiliary_Sphere\",6378137,0]],\
/// // //	//	PRIMEM[\"Greenwich\",0],\
/// // //	//	UNIT[\"degree\",0.0174532925199433]],\
/// // //	//	PROJECTION[\"Mercator_1SP\"],\
/// // //	//	PARAMETER[\"central_meridian\",0],\
/// // //	//	PARAMETER[\"scale_factor\",1],\
/// // //	//	PARAMETER[\"false_easting\",0],\
/// // //	//	PARAMETER[\"false_northing\",0],\
/// // //	//	UNIT[\"metre\",1,\
/// // //	//	AUTHORITY[\"EPSG\",\"9001\"]]]";
////poDstDS->SetProjection( wgs84_merc );
//
////从mainMap坐标转换到wgs84_merc坐标
////OGRSpatialReference wgs84(wgs84_merc);
/// *OGRSpatialReference wgs84(_srsWKT.c_str());
// OGRSpatialReference localSRS(_srsWKT.c_str());
// OGRCoordinateTransformation* coordTrans = OGRCreateCoordinateTransformation(&localSRS, &wgs84);
// coordTrans->Transform(1, topLeft, topLeft + 1);
// CPLFree(coordTrans);*/
//
//// 2. 直接使用mainMap的投影，无需进行坐标转换
// poDstDS->SetProjection( _srsWKT.c_str() );
//
//// Affine information
// double adfGeoTransform[6] = { topLeft[0], 1 / _pixelPerMeter, 0, topLeft[1], 0, -1 / _pixelPerMeter };
// poDstDS->SetGeoTransform(adfGeoTransform);
//
// GDALClose( (GDALDatasetH) poDstDS );
// }

//
// class CameraSyncCallback:public osg::NodeCallback
// {
// public:
// CameraSyncCallback(osg::Camera* camera): slaveCamera(camera)
// {
// if (slaveCamera.valid())
// {
// slaveCamera->getViewMatrixAsLookAt(_eye, _center, _up);
// }
// }
//
// virtual ~CameraSyncCallback()
// {   }
//
// virtual void operator()(osg::Node* node,osg::NodeVisitor* nv)
// {
// osg::Camera* camera = dynamic_cast<osg::Camera*>(node);
// if(camera && slaveCamera.valid())
// {
//// 只改变视角旋转方向，视点和视线方向均不变
// osg::Vec3 eye,center,up;
// camera->getViewMatrixAsLookAt(eye, center, up);
// if (eye.z() < 0.0)
// {
// up = -up;
// }
// up.z() = 0;
// if (up.x() == up.y() == 0.0)
// up.y() = 1;
// slaveCamera->setViewMatrixAsLookAt(_eye, _center, up);
// }
// traverse(node, nv);
// }
// private:
// osg::ref_ptr<osg::Camera> slaveCamera;
// osg::Vec3 _eye;
// osg::Vec3 _center;
// osg::Vec3 _up;
// };

// void CaptureThread::run()
// {
// emit isWorking(true);
//
// osgViewer::Viewer& viewer = _viewer;
// osg::Camera* camera = viewer.getCamera();
// osg::ref_ptr<CustomRenderer> renderer = new CustomRenderer( camera );
////camera->setRenderer( renderer.get() );
// viewer.setThreadingModel( osgViewer::Viewer::SingleThreaded );
//
//// Realize and initiate the first PagedLOD request
// viewer.realize();
// viewer.frame();
//
// _printer->init( camera );
// while ( !_printer->done() )
// {
// viewer.advance();
//
//// Keep updating and culling until full level of detail is reached
// renderer->setCullOnly( true );
// while ( viewer.getDatabasePager()->getRequestsInProgress() )
// {
// viewer.updateTraversal();
// viewer.renderingTraversals();
// }
//
// renderer->setCullOnly( false );
////try{
// _printer->frame( viewer.getFrameStamp(), viewer.getSceneData() );
/// *}
// catch(std::bad_alloc&)
// {
// emit aborted(tr("终止，内存不足！"));
// terminate();
// }*/
// viewer.renderingTraversals();
// }
// emit isWorking(false);
// }

// _captureRoot->setValue(0, false);
// _captureRoot->setValue(1, true);

// waitForCapturing(true);

// osg::ref_ptr<CustomRenderer> renderer = new CustomRenderer( _orthoCamera );
// _orthoCamera->setRenderer( renderer.get() );

// _printer->init( _orthoCamera );
// while ( !_printer->done() )
// {
// _viewer.advance();

//// Keep updating and culling until full level of detail is reached
// renderer->setCullOnly( true );
// while ( _viewer.getDatabasePager()->getRequestsInProgress() )
// {
// _viewer.updateTraversal();
// _viewer.renderingTraversals();
// }

// renderer->setCullOnly( false );
////try{
// _printer->frame( _viewer.getFrameStamp(), _viewer.getSceneData() );
/// *}
// catch(std::bad_alloc&)
// {
// emit aborted(tr("终止，内存不足！"));
// terminate();
// }*/
// _viewer.renderingTraversals();
// }

//

// waitForCapturing(false);
