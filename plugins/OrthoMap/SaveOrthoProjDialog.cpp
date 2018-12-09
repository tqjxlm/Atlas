#include "SaveOrthoProjDialog.h"

#include <iostream>
using namespace std;

#include <QDialog>
#include <QThread>
#include <QTimer>
#include <QFileDialog>

#include <osg/ComputeBoundsVisitor>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>

#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

static const double captureZOffset = 50;
static const osg::Vec4 bgColor = { 0, 0, 0, 0 };

SaveOrthoProjDialog::SaveOrthoProjDialog(ViewerWidget& vw, osg::Node* scene, std::string wkt, ProjectionMode mode, QWidget* parent)
  : QDialog(parent),
  _vw(vw),
  _view(*vw.getMainView()),
  _scene(scene->asTransform()->asPositionAttitudeTransform()),
  _srsWKT(wkt),
  _successed(false),
  _finished(false),
  _mode(mode)
{
  _ui.setupUi(this);
  connect(_ui.pushButtonOk, SIGNAL(clicked()), this, SLOT(startCapturing()));
  connect(_ui.tileNumSpinBox, SIGNAL(valueChanged(int)), _ui.tileNumSpinBox_2, SLOT(setValue(int)));
  this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
  _ui.tileNumSpinBox->setValue(1);
  _ui.pixelSpinBox->setValue(0.2);
}

SaveOrthoProjDialog::~SaveOrthoProjDialog()
{
  if (!_finished)
    finish();
}

void SaveOrthoProjDialog::setup()
{
  // Show the dialog
  osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
  unsigned int screenWidth, screenHeight;
  wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
  this->move((screenWidth - this->width()) / 2, (screenHeight - this->height()) / 2);
  this->setWindowFlags(this->windowFlags() & ~Qt::CustomizeWindowHint & ~Qt::WindowMinMaxButtonsHint);
  this->setWindowModality(Qt::WindowModal);
  this->show();
}

void SaveOrthoProjDialog::doMosaic()
{
  if (_ui.mergeCheckBox->isChecked())
  {
    WaitProgressDialog* waitDialog = new WaitProgressDialog(tr("Merging tiles...") + " (gdalbuildvrt.exe)",
      "", 0, 1, 0, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    waitDialog->adjustSize();
    waitDialog->setWindowModality(Qt::WindowModal);

    // Use gdalbuildvrt.exe to do the mosaic, generating a temprary vrt file
    QStringList commandList;
    commandList.push_back("-a_srs");
    commandList.push_back(_srsWKT.c_str());
    commandList.push_back("-overwrite");
    commandList.push_back("output.vrt");
    commandList.append(_fileList);

    QStringList environment;
    environment.push_back("GDAL_DATA=./GDAL/data");

    QProcess *process = new QProcess(this);
    process->setWorkingDirectory(_path);
    cout << "cd " << _path.toStdString() << endl;
    cout << "gdalbuildvrt " << commandList.join(' ').toStdString() << endl;
    process->setStandardOutputFile(_path + "/log.txt");
    process->setStandardErrorFile(_path + "/error.txt");
    process->setEnvironment(environment);

    // Generate the output image with vrt file
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
      [=](int exitCode, QProcess::ExitStatus exitStatus) {
      waitDialog->close();
      doTranslate();
    });
    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
      QMessageBox::critical(nullptr, tr("Failed to mosaic"), process->errorString());
      waitDialog->close();
    });
    process->start("gdalbuildvrt.exe", commandList);
    waitDialog->exec();
  }
}

void SaveOrthoProjDialog::doTranslate()
{
  WaitProgressDialog* waitDialog = new WaitProgressDialog(tr("Merging tiles...") + " (gdal_translate.exe)",
    "", 0, 1, 0, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  waitDialog->adjustSize();
  waitDialog->setWindowModality(Qt::WindowModal);

  // Convert to tiff file using gdal_translate
  QStringList commandList;
  commandList.push_back("-of");
  commandList.push_back("GTiff");
  commandList.push_back("-co");
  commandList.push_back("TILED=YES");
  commandList.push_back("-co");
  commandList.push_back("NUM_THREADS=ALL_CPUS");
  if (_mode == DOM)
  {
    commandList.push_back("-co");
    commandList.push_back("COMPRESS=JPEG");
  }
  commandList.push_back("-a_srs");
  commandList.push_back(_srsWKT.c_str());
  commandList.push_back("output.vrt");
  commandList.push_back("output." + _extension);

  QStringList environment;
  environment.push_back("GDAL_DATA=./GDAL/data");

  QProcess *process = new QProcess(this);
  process->setWorkingDirectory(_path);
  cout << "gdal_translate " << commandList.join(' ').toStdString() << endl;
  process->setStandardOutputFile(_path + "/log.txt");
  process->setStandardErrorFile(_path + "/error.txt");
  process->setEnvironment(environment);

  connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
    [=](int exitCode, QProcess::ExitStatus exitStatus) {
    waitDialog->close();
    //buildOverview();
  });
  connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
    QMessageBox::critical(nullptr, tr("Failed to tranlate"), process->errorString());
    waitDialog->close();
  });
  process->start("gdal_translate.exe", commandList);
  waitDialog->exec();
}

void SaveOrthoProjDialog::buildOverview()
{
  WaitProgressDialog* waitDialog = new WaitProgressDialog(tr("Building overview..."),
    "", 0, 1, 0, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  waitDialog->adjustSize();
  waitDialog->setWindowModality(Qt::WindowModal);

  // Build overviews
  QStringList commandList;
  commandList.push_back("-r");
  commandList.push_back("average");
  commandList.push_back("output." + _extension);

  QStringList environment;
  environment.push_back("GDAL_DATA=./GDAL/data");

  QProcess *process = new QProcess(this);
  process->setWorkingDirectory(_path);
  cout << "gdaladdo " << commandList.join(' ').toStdString() << endl;
  process->setStandardOutputFile(_path + "/log.txt");
  process->setStandardErrorFile(_path + "/error.txt");
  process->setEnvironment(environment);

  connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
    [=](int exitCode, QProcess::ExitStatus exitStatus) {
    waitDialog->close();
  });
  connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
    QMessageBox::critical(nullptr, tr("Failed to build overview"), process->errorString());
    waitDialog->close();
  });
  process->start("gdaladdo.exe", commandList);
  waitDialog->exec();
}

void SaveOrthoProjDialog::processTile(osg::MatrixTransform* subScene, std::string nodeName)
{
  QString tileName = QString::fromStdString(nodeName).split('/').back().split('.').front();
  QString saveName = _path + "/" + tileName + '.' + _extension;
  _fileList.push_back(tileName + '.' + _extension);

  advanceCapturing(tr("Processing ") + tileName);
  if (!_ui.overwriteCheckBox->isChecked() && QFileInfo(saveName).exists())
  {
    cout << "Tile exist, skipped: " << tileName.toStdString() << endl;
    return;
  }
  cout << "Tile export started: " << tileName.toStdString() << endl;

  osg::ComputeBoundsVisitor boundsVisitor;
  boundsVisitor.apply(*subScene);
  auto bounding = boundsVisitor.getBoundingBox();
  float boundingW = bounding.xMax() - bounding.xMin() + 1;
  float boundingH = bounding.yMax() - bounding.yMin() + 1;
  unsigned int viewW = boundingW + 1;
  unsigned int viewH = boundingH + 1;

  // Calculate view size
  unsigned int widgetWidth, widgetHeight;
  widgetWidth = _vw.width();
  widgetHeight = _vw.height();
  if (viewW >= widgetWidth - 50)
  {
    viewH = (double)viewH / viewW * (widgetWidth - 50);
    viewW = widgetWidth - 50;
  }
  if (viewH >= widgetHeight - 50)
  {
    viewW = (double)viewW / viewH * (widgetHeight - 50);
    viewH = widgetHeight - 50;
  }

  // Set camera attributes for capturing
  osg::Vec3 eye, center, up;
  eye = bounding.center();

  // A very large positive z offset to make sure the camera is above the whole model
  eye.z() = bounding.zMax() + captureZOffset;
  center = eye - osg::Vec3(0, 0, 1);
  up = osg::Vec3(0, 1, 0);

  _orthoCamera->setViewport((widgetWidth - 50 - viewW) / 2, 25, viewW, viewH);
  _orthoCamera->setViewMatrixAsLookAt(eye, center, up);
  _orthoCamera->setProjectionMatrixAsOrtho(-boundingW / 2, boundingW / 2,
    -boundingH / 2, boundingH / 2, 1, 10000);
  _orthoCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
  _orthoCamera->setClearColor(bgColor);

  // Init capture root
  osg::ref_ptr<osg::Switch> captureRoot = new osg::Switch;
  captureRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF & osg::StateAttribute::OVERRIDE);
  captureRoot->addChild(subScene, true);
  _view.setSceneData(captureRoot);
  _view.setCameraManipulator(0);

  // Ouput settings
  float tileWidth = boundingW / _numTiles * _pixelPerMeter + 1;
  float tileHeight = boundingH / _numTiles * _pixelPerMeter + 1;
  int posterWidth = tileWidth * _numTiles;
  int posterHeight = tileHeight * _numTiles;

  // Init off-screen rendering camera
  osg::ref_ptr<osg::Camera> captureCamera = new osg::Camera;
  captureCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  captureCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  captureCamera->setRenderOrder(osg::Camera::PRE_RENDER);
  osg::Camera::RenderTargetImplementation renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT;
  captureCamera->setRenderTargetImplementation(renderImplementation);
  captureCamera->setViewport(0, 0, tileWidth, tileHeight);
  captureCamera->addChild(subScene);
  captureCamera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
  captureRoot->addChild(captureCamera, true);

  // Init output printer
  osg::ref_ptr<PosterPrinter> printer = new PosterPrinter;
  printer->setTileSize(tileWidth, tileHeight);
  printer->setPosterSize(posterWidth, posterHeight);
  printer->setOutputType(_mode == DOM ? PosterPrinter::RGB : PosterPrinter::DEPTH);
  printer->setCamera(captureCamera);
  printer->setOutputTiles(false);
  printer->setOutputTileExtension(_extension.toStdString());
  printer->setPixel(_pixelPerMeter);

  // Prepare the output image
  osg::ref_ptr<osg::Image> posterImage = 0;
  posterImage = new osg::Image;
  if (_mode == DOM)
    posterImage->allocateImage(posterWidth, posterHeight, 1, GL_RGB, GL_UNSIGNED_BYTE);
  else
    posterImage->allocateImage(posterWidth, posterHeight, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
  printer->setFinalPoster(posterImage.get());
  printer->setOutputPosterName(_path.toLocal8Bit().toStdString());
  printer->setPath(_path);
  printer->setViewCamera(_orthoCamera);

  // Begin capturing
  printer->init(_view.getCamera());

  osg::Camera* camera = _view.getCamera();
  osg::ref_ptr<CustomRenderer> renderer = new CustomRenderer(camera);
  camera->setRenderer(renderer.get());

  while (!printer->done())
  {
    _vw.advance();

    // Keep updating and culling until full level of detail is reached
    renderer->setCullOnly(true);
    while (_view.getDatabasePager()->getRequestsInProgress())
    {
      _vw.updateTraversal();
      _vw.renderingTraversals();
    }

    renderer->setCullOnly(false);
    printer->frame(_view.getFrameStamp(), _view.getSceneData());
    _vw.renderingTraversals();
  }
  writeWithGDAL(tileName.toLocal8Bit().toStdString(), saveName.toLocal8Bit().toStdString(),
    printer->getFinalPoster()->data(), bounding,
    posterWidth, posterHeight);
}

void SaveOrthoProjDialog::startCapturing()
{
  _path = QFileDialog::getExistingDirectory(0, tr("Choose save location"), ".");
  if (_path.isEmpty())
    return;
  _numTiles = _ui.tileNumSpinBox->value();
  _pixelPerMeter = 1 / _ui.pixelSpinBox->value();

  _waitDialog = new WaitProgressDialog("", "", 0, _scene->getNumChildren(), this, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  connect(this, SIGNAL(updateWaitMessage(const QString&)), _waitDialog, SLOT(updateMessage(const QString&)));
  connect(this, SIGNAL(updateTime()), _waitDialog, SLOT(updateTime()));
  _waitDialog->adjustSize();
  _waitDialog->setWindowModality(Qt::WindowModal);
  _waitDialog->show();

  this->hide();
  _view.getDatabasePager()->setDoPreCompile(false);

  // TODO: Use more lod to accelerate the process
  //_origMaxLOD = _view.getDatabasePager()->getTargetMaximumNumberOfPageLOD();
  //_view.getDatabasePager()->setTargetMaximumNumberOfPageLOD(2000);
  _vw.stopRendering();

  // Save the original view states
  _orthoCamera = _view.getCamera();
  _origColor = _orthoCamera->getClearColor();
  _origViewWidth = _orthoCamera->getViewport()->width();
  _origViewHeight = _orthoCamera->getViewport()->height();
  _origViewMatrix = _orthoCamera->getViewMatrix();
  _origProjMatrix = _orthoCamera->getProjectionMatrix();
  _origNearFarMode = _orthoCamera->getComputeNearFarMode();
  _origManip = _view.getCameraManipulator();
  _origScene = _view.getSceneData();

  // Timer that show on the waiting dialog
  QTimer* timer = new QTimer;
  QThread* timerThread = new QThread();
  timer->setInterval(1000);
  timer->moveToThread(timerThread);
  connect(timer, &QTimer::timeout, _waitDialog, &WaitProgressDialog::updateTime);
  connect(timerThread, SIGNAL(started()), timer, SLOT(start()));
  connect(timerThread, &QThread::finished, timer, &QObject::deleteLater);
  connect(timerThread, &QThread::finished, timerThread, &QObject::deleteLater);
  timerThread->start();

  // Get world transform
  double modelZOffset = 0;
  _scene->getUserValue<double>("zOffset", modelZOffset);
  osg::Matrix worldMatrix = osg::computeLocalToWorld(_scene->getParentalNodePaths()[0]);
  worldMatrix.postMultTranslate(osg::Vec3d(0, 0, -modelZOffset));
  auto offset = worldMatrix.getTrans();

  osg::ref_ptr<osg::MatrixTransform> subScene = new osg::MatrixTransform();
  subScene->setMatrix(worldMatrix);
  subScene->addChild(_scene);

  // Process tile by tile
  for (unsigned int i = 0; i < _scene->getNumChildren(); i++)
  {
    subScene->removeChildren(0, subScene->getNumChildren());
    subScene->addChild(_scene->getChild(i));
    processTile(subScene, _scene->getChild(i)->getName());
  }

  _successed = true;
  timerThread->quit();
  emit accepted();
}

void SaveOrthoProjDialog::finish()
{
  if (_successed)
  {
    // Mosaic
    doMosaic();

    QMessageBox::information(this, tr("Exporting completed"), tr("Exported to ") + _path, QMessageBox::NoButton);
    _waitDialog->close();

    // Recover the original view states
    _vw.startRendering();

    _view.getDatabasePager()->setTargetMaximumNumberOfPageLOD(_origMaxLOD);
    _view.setCameraManipulator(_origManip);
    _view.setSceneData(_origScene);

    _orthoCamera->setViewport(0, 0, _origViewWidth, _origViewHeight);
    _orthoCamera->setProjectionMatrix(_origProjMatrix);
    _orthoCamera->setViewMatrix(_origViewMatrix);
    _orthoCamera->setComputeNearFarMode(_origNearFarMode);
    _orthoCamera->setClearColor(_origColor);
  }

  _finished = true;
}

void SaveOrthoProjDialog::captureFailed(const QString& msg)
{
  QMessageBox::critical(this, tr("Error"), tr("Insufficient memory. Please lower pixel number or increase tile number and retry."));
  _waitDialog->close();

  this->show();
}

void SaveOrthoProjDialog::advanceCapturing(const QString& msg)
{
  if (!_successed)
  {
    _msg = msg;
    _waitDialog->setValue(_waitDialog->value() + 1);
    emit updateWaitMessage(msg);
  }
}

void SaveOrthoProjDialog::writeWithGDAL(std::string fileName, std::string saveName, const unsigned char *data, osg::BoundingBox bounding, int width, int height)
{
  GDALAllRegister();

  // Default to GTiff driver, other drivers are also possible
  // More details: http://www.gdal.org/frmt_gtiff.html
  const char *pszFormat = "GTiff";
  GDALDriver *poDriver;
  poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

  char **papszOptions = NULL;
  //papszOptions = CSLSetNameValue(papszOptions, "TILED", "YES");

  // Create the file and copy raster data
  GDALDataset *poDstDS;
  if (_mode == DOM)
  {
    poDstDS = poDriver->Create(saveName.c_str(), width, height,
      3, GDT_Byte, papszOptions);
    poDstDS->RasterIO(GF_Write, 0, 0, width, height,
      (unsigned char*)data + width * (height - 1) * 3,
      width, height, GDT_Byte, 3, nullptr,
      3, -width * 3, 1);
    poDstDS->GetRasterBand(1)->SetNoDataValue(0);
    poDstDS->GetRasterBand(2)->SetNoDataValue(0);
    poDstDS->GetRasterBand(3)->SetNoDataValue(0);
  }
  else
  {
    poDstDS = poDriver->Create(saveName.c_str(), width, height,
      1, GDT_Float32, papszOptions);
    poDstDS->RasterIO(GF_Write, 0, 0, width, height,
      (float*)data + width * (height - 1),
      width, height, GDT_Float32, 1, nullptr,
      sizeof(float), -width * sizeof(float), width * height * sizeof(float));
    poDstDS->GetRasterBand(1)->SetNoDataValue(10000);
  }

  // Set affine information with origin default to top left
  double topLeft[2] = { bounding.corner(2)[0], bounding.corner(2)[1] };
  double adfGeoTransform[6] = { topLeft[0], 1 / _pixelPerMeter, 0, topLeft[1], 0, -1 / _pixelPerMeter };
  poDstDS->SetProjection(_srsWKT.c_str());
  poDstDS->SetGeoTransform(adfGeoTransform);

  GDALClose((GDALDatasetH)poDstDS);
}
