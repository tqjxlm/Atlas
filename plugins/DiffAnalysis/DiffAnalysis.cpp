#include "DiffAnalysis.h"

#include <vector>
#include <iostream>
using namespace std;

#include <QDoubleSpinBox>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QToolBar>
#include <QDialogButtonBox>

#include <osg/LineWidth>
#include <osg/ComputeBoundsVisitor>
#include <osgEarth/SpatialReference>
#include <osgSim/OverlayNode>

// GDAL
#include <gdal_priv.h>
#include <gdal_alg.h>
#include <cpl_string.h>
#include <ogr_spatialref.h>
#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_feature.h>

#include "DiffVisitor.h"
#include <ViewerWidget/ViewerWidget.h>

struct GridPoint
{
  int  x, y;
};

static int  coreX[3][3] = {
	{ -1, 0, 1 },
	{ -2, 0, 2 },
	{ -1, 0, 1 }
};
static int  coreY[3][3] = {
	{ -1, -2, -1 },
  {  0,  0,  0 },
  {  1,  2,  1 }
};
static int  coreFull[3][3] = {
	{ 1, 1, 1 },
	{ 1, 0, 1 },
	{ 1, 1, 1 }
};

inline int  applyCore(osg::Image *image, int core[][3], int r, int w)
{
  int  sum = 0;

	for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (core[i][j] != 0)
      {
        sum += *(char *)image->data(w + j - 1, r + i - 1) * core[i][j];
      }
    }
  }

	return sum;
}

inline float  applyCoref(osg::Image *image, int core[][3], int r, int w)
{
  int  sum = 0;

	for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (core[i][j] != 0)
      {
        sum += *(float *)image->data(w + j - 1, r + i - 1) * core[i][j];
      }
    }
  }

	return sum;
}

inline bool  shouldSet(osg::Image *image, int r, int w, int flag)
{
  if (*(char *)image->data(w, r) == 0)
  {
		return false;
  }

  int  p[8];
  p[0] = *(char *)image->data(w, r - 1);
  p[1] = *(char *)image->data(w + 1, r - 1);
  p[2] = *(char *)image->data(w + 1, r);
  p[3] = *(char *)image->data(w + 1, r + 1);
  p[4] = *(char *)image->data(w, r + 1);
  p[5] = *(char *)image->data(w - 1, r + 1);
  p[6] = *(char *)image->data(w - 1, r);
  p[7] = *(char *)image->data(w - 1, r - 1);

  int  sum   = p[0];
  int  count = 0;

	for (int i = 1; i < 8; i++)
	{
		if (p[i] > p[i - 1])
    {
      count++;
    }

    sum += p[i];
	}

	if (p[0] > p[7])
  {
    count++;
  }

  if ((sum < 2) || (sum > 6) || (count != 1))
  {
		return false;
  }

	if (flag == 0)
	{
    if ((p[0] * p[2] * p[4] != 0) || (p[6] * p[2] * p[4] != 0))
    {
			return false;
    }
  }
	else
	{
    if ((p[0] * p[2] * p[6] != 0) || (p[0] * p[4] * p[6] != 0))
    {
			return false;
    }
  }

	return true;
}

typedef vector<int> ivChainCode;

// Trace contour
// 1. pImageData   image data
// 2. nWidth       image width
// 3. nHeight      image height
// 4. nWidthStep   image width step
// 5. pStart       starting point
// 6. pChainCode   chain code
bool  TracingContour(unsigned char *pImageData, int nWidth, int nHeight, int nWidthStep,
                     GridPoint *pStart, bool *valid, ivChainCode *pChainCode)
{
  int              i           = 0;
  int              j           = 0;
  int              k           = 0;
  int              x           = 0;
  int              y           = 0;
  bool             bTracing    = false;
  GridPoint        ptCurrent   = { 0, 0 };
  GridPoint        ptTemp      = { 0, 0 };
  unsigned char   *pLine       = NULL;
  const GridPoint  ptOffset[8] = {
    {  1, 0 }, {  1,  1 }, { 0,  1 }, { -1,  1 },
    { -1, 0 }, { -1, -1 }, { 0, -1 }, {  1, -1 }
	};

	pStart->x = 0;
	pStart->y = 0;
	pChainCode->clear();

	// The starting point
	for (y = 0; y < nHeight; y++)
	{
		pLine = pImageData + nWidthStep * y;

		for (x = 0; x < nWidth; x++)
		{
			if (pLine[x] == 1)
			{
        bTracing    = true;
        pStart->x   = x;
        pStart->y   = y;
				ptCurrent.x = x;
				ptCurrent.y = y;
			}
		}
	}

  if ((pStart->x == 0) && (pStart->y == 0))
  {
		return false;
  }

	// Tracing
	while (bTracing)
	{
		bTracing = false;

		for (i = 0; i < 8; i++, k++)
		{
			k &= 0x07;
      x  = ptCurrent.x + ptOffset[k].x;
      y  = ptCurrent.y + ptOffset[k].y;

      if ((x >= 0) && (x < nWidth) && (y >= 0) && (y < nHeight))
			{
				// Check if it is a contour point
				if (pImageData[nWidthStep * y + x] == 1)
				{
					for (j = 0; j < 8; j += 2)
					{
						ptTemp.x = x + ptOffset[j].x;
						ptTemp.y = y + ptOffset[j].y;

            if ((ptTemp.x >= 0) && (ptTemp.x < nWidth)
                && (ptTemp.y >= 0) && (ptTemp.y < nHeight))
						{
							if (pImageData[nWidthStep * ptTemp.y + ptTemp.x] == 0)
							{
                bTracing    = true;
								ptCurrent.x = x;
								ptCurrent.y = y;
								pChainCode->push_back(k);
								break;
							}
						}
					}
				}
			}

			if (bTracing)
			{
				// If returning to starting point, finish tracing
        if ((pStart->x == ptCurrent.x) && (pStart->y == ptCurrent.y))
				{
					bTracing = false;
          *valid   = true;
				}

				break;
			}
		}

		k += 0x06;
	}

	return true;
}

DiffAnalysis::DiffAnalysis():
	_diffThreashold(0.10f),
	_captureZOffset(500),
	_pixelPerMeter(5)
{
  _pluginName     = tr("Difference Analysis");
	_pluginCategory = "Analysis";
}

DiffAnalysis::~DiffAnalysis()
{
}

void  DiffAnalysis::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("diffAnalysisAction"));
	_action->setCheckable(true);
	_action->setEnabled(false);
  QIcon  icon32;
	icon32.addFile(QStringLiteral("resources/icons/compare.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon32);
	_action->setVisible(true);
	_action->setText(tr("Diff"));
	_action->setToolTip(tr("Difference Analysis"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void  DiffAnalysis::onDoubleClick()
{
	if (_isDrawing)
	{
		endDrawing();
		_currentDrawNode->addDrawable(createPointGeode(_endPoint, _intersections.begin()->getLocalIntersectNormal()));

    // _currentDrawNode->removeChildren(0, _currentDrawNode->getNumChildren());

		testDifference();

    // showContour(osgDB::readImageFile("./contour.tiff.gdal"));

    // int index;
    // if (!_currentDrawNode->getUserValue("index", index))
    // return;
    // osg::ref_ptr<osg::Geometry> polyg = generateContour(_contour);

    // _currentDrawNode->setNodeMask(SHOW_IN_WINDOW_1 << 1);

    // _currentAnchor->removeChild(_currentDrawNode);

    // removeOverlay(index);

		recordCurrent();
	}
}

void  DiffAnalysis::onLeftButton()
{
	if (!_isDrawing)
	{
		beginDrawing();
    _contour     = new osg::Vec3Array;
		_captureRoot = NULL;
	}

	_contour->push_back(_currentWorldPos);
	DrawSurfacePolygon::onLeftButton();
}

void  DiffAnalysis::testDifference()
{
	// Prompt for parameters
	if (promptForUserSettings() == QDialog::Rejected)
  {
    return;
  }

	initParameters();

	// Use 2 views to render 2 depth map, respectively
  auto  view1 = _mainViewer->getMainView();
  auto  view2 = _mainViewer->getView(_mainViewer->getNumViews() - 1);

  osg::ref_ptr<osg::Image>  image1 = initDepthRenderer(view1);
  osg::ref_ptr<osg::Image>  image2 = initDepthRenderer(view2);

	_mainViewer->frame();
	_mainViewer->frame();

	// Convert depth to elevation
	calcuZvaluefromDepth(image1, view1->getCamera());
	calcuZvaluefromDepth(image2, view2->getCamera());

	// Compare the depth maps, generate a binary difference map depending on the difference
  osg::ref_ptr<osg::Image>  diffMap = new osg::Image;
	diffMap->allocateImage(_posterWidth, _posterHeight, 1, GL_DEPTH_COMPONENT, GL_FLOAT);

  int  numDiffPoints = 0;

	for (int i = _posterHeight - 1; i >= 0; i--)
	{
		for (int j = 0; j < _posterWidth; j++)
		{
      *(float *)diffMap->data(j, i) = abs(*(float *)image1->data(j, i) - *(float *)image2->data(j, i)) < _diffThreashold ? 0.0f : 1.0f;
		}
	}

	cout << "There are " << numDiffPoints << " different points in diffMap." << endl;

	// Get contour of the difference map
  osg::ref_ptr<osg::Image>  contourMap = analyzeContour(diffMap);
	writeWithGDAL(contourMap, "./contour.tiff");
	showContour(contourMap);

	recoverView(view1);
	recoverView(view2);
}

osg::ref_ptr<osg::Image>  DiffAnalysis::initDepthRenderer(osgViewer::View *view)
{
	// Backup view status
  osg::ref_ptr<osg::Camera>  orthoCamera = view->getCamera();
  ViewStatus                 origStatus;
  origStatus.origViewWidth   = orthoCamera->getViewport()->width();
  origStatus.origViewHeight  = orthoCamera->getViewport()->height();
  origStatus.origViewMatrix  = orthoCamera->getViewMatrix();
  origStatus.origProjMatrix  = orthoCamera->getProjectionMatrix();
	origStatus.origNearFarMode = orthoCamera->getComputeNearFarMode();
  origStatus.origManip       = view->getCameraManipulator();
  origStatus.origScene       = view->getSceneData();
  _viewStatus[view]          = origStatus;

	view->setCameraManipulator(0);

	// Init an ortho camera
  osg::Vec3  eye, center, up;
  eye     = _boundingBox.center();
	eye.z() = _boundingBox.zMax() + _captureZOffset;
  center  = eye - osg::Vec3(0, 0, 1);
  up      = osg::Vec3(0, 1, 0);

  float  boundingWidth  = _boundingBox.xMax() - _boundingBox.xMin() + 1;
  float  boundingHeight = _boundingBox.yMax() - _boundingBox.yMin() + 1;
	orthoCamera->setViewport(50, 50, _viewWidth, _viewHeight);
	orthoCamera->setViewMatrixAsLookAt(eye, center, up);
	orthoCamera->setProjectionMatrixAsOrtho(-boundingWidth / 2, boundingWidth / 2,
                                          -boundingHeight / 2, boundingHeight / 2, 1, 1000);
	orthoCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

	// Init a depth camera
  osg::ref_ptr<osg::Camera>  captureCamera = new osg::Camera;
	captureCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	captureCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	captureCamera->setRenderOrder(osg::Camera::PRE_RENDER);
  osg::Camera::RenderTargetImplementation  renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT;
	captureCamera->setRenderTargetImplementation(renderImplementation);
	captureCamera->setViewport(0, 0, _posterWidth, _posterHeight);
	captureCamera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
	captureCamera->setCullMask(orthoCamera->getCullMask());
	captureCamera->setViewMatrix(orthoCamera->getViewMatrix());
	captureCamera->setProjectionMatrix(orthoCamera->getProjectionMatrix());
	captureCamera->addChild(_overlayNode);

	// Reset rendering nodes of the scene
  osg::ref_ptr<osg::Group>  captureRoot = new osg::Group;
	captureRoot->addChild(_overlayNode);
	captureRoot->addChild(captureCamera);
	view->setSceneData(captureRoot);

	// Bind depth buffer to an image
  osg::ref_ptr<osg::Image>  posterImage = new osg::Image;
	posterImage->allocateImage(_posterWidth, _posterHeight, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
	captureCamera->attach(osg::Camera::DEPTH_BUFFER, posterImage.get(), 0, 0);

	return posterImage;
}

void  DiffAnalysis::recoverView(osgViewer::View *view)
{
  auto  origStatus = _viewStatus[view];

	view->setSceneData(origStatus.origScene);
	view->setCameraManipulator(origStatus.origManip);

  auto  camera = view->getCamera();
	camera->setViewport(0, 0, origStatus.origViewWidth, origStatus.origViewHeight);
	camera->setProjectionMatrix(origStatus.origProjMatrix);
	camera->setViewMatrix(origStatus.origViewMatrix);
	camera->setComputeNearFarMode(origStatus.origNearFarMode);

	_viewStatus.erase(_viewStatus.find(view));
}

void  DiffAnalysis::initParameters()
{
	// Generate a bounding box according to the selected area
  auto  polyg = tesselatedPolygon(_contour);

  osg::ComputeBoundsVisitor  boundsVisitor;
	polyg->accept(boundsVisitor);
	_boundingBox = boundsVisitor.getBoundingBox();
  float  boundingWidth  = _boundingBox.xMax() - _boundingBox.xMin() + 1;
  float  boundingHeight = _boundingBox.yMax() - _boundingBox.yMin() + 1;

	// Compute view size according to the bounding
  _viewWidth  = boundingWidth + 1;
	_viewHeight = boundingHeight + 1;

  unsigned int  widgetWidth, widgetHeight;
  widgetWidth  = _mainViewer->width();
	widgetHeight = _mainViewer->height();

	if (_viewWidth >= widgetWidth - 50)
	{
		_viewHeight = (double)_viewHeight / _viewWidth * (widgetWidth - 50);
    _viewWidth  = widgetWidth - 50;
	}

	if (_viewHeight >= widgetHeight - 50)
	{
    _viewWidth  = (double)_viewWidth / _viewHeight * (widgetHeight - 50);
		_viewHeight = widgetHeight - 50;
	}

	// Tile parameters to decide the image size
  int  numTiles   = 1;
  int  tileWidth  = boundingWidth / numTiles * _pixelPerMeter + 1;
  int  tileHeight = boundingHeight / numTiles * _pixelPerMeter + 1;
  _posterWidth  = tileWidth * numTiles;
	_posterHeight = tileHeight * numTiles;
}

void  DiffAnalysis::showContour(osg::Image *image)
{
  osg::ref_ptr<osg::Geometry>  geom = new osg::Geometry;
	_contourPoints = new osg::Vec3Array;

  GridPoint        ptStart     = { 0, 0 };
  GridPoint        ptCurrent   = { 0, 0 };
  const GridPoint  ptOffset[8] = {
    {  1, 0 }, {  1,  1 }, { 0,  1 }, { -1,  1 },
    { -1, 0 }, { -1, -1 }, { 0, -1 }, {  1, -1 }
	};
  bool             valid = false;
  ivChainCode      chainCode;
  int              nWidthStep = _posterWidth;

  while (TracingContour((unsigned char *)image->data(), _posterWidth, _posterHeight, nWidthStep, &ptStart, &valid, &chainCode))
	{
    ptCurrent.x                            = ptStart.x;
    ptCurrent.y                            = ptStart.y;
		*image->data(ptCurrent.x, ptCurrent.y) = 0;

		for (ivChainCode::iterator i = chainCode.begin(); i != chainCode.end(); i++)
		{
      ptCurrent.x                           += ptOffset[*i].x;
      ptCurrent.y                           += ptOffset[*i].y;
			*image->data(ptCurrent.x, ptCurrent.y) = 0;
		}

    if (valid && (chainCode.size() > 10))
		{
			geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, _contourPoints->size(), chainCode.size() + 1));

			// Render the contour
			ptCurrent.x = ptStart.x;
			ptCurrent.y = ptStart.y;
			_contourPoints->push_back(getTranslatedPoint(ptCurrent.x, ptCurrent.y) - _anchoredOffset);

			for (ivChainCode::iterator i = chainCode.begin(); i != chainCode.end(); i++)
			{
				ptCurrent.x += ptOffset[*i].x;
				ptCurrent.y += ptOffset[*i].y;
				_contourPoints->push_back(getTranslatedPoint(ptCurrent.x, ptCurrent.y) - _anchoredOffset);
			}
		}

		valid = false;
	}

  drawOverlay();
}

int  DiffAnalysis::promptForUserSettings()
{
  osg::ref_ptr<osgGA::CameraManipulator>  manip = _mainViewer->getMainView()->getCameraManipulator();
	_mainViewer->getMainView()->setCameraManipulator(0);

  QDialog *settingsDialog = new QDialog(_mainViewer);
	settingsDialog->setWindowTitle(tr("Parameter settings"));
	settingsDialog->setMinimumWidth(200);

  QGridLayout *mainLayout    = new QGridLayout(settingsDialog);
  QLabel      *accuarcyLabel = new QLabel(tr("Pixel Accuaracy (m)"));
	mainLayout->addWidget(accuarcyLabel, 0, 0);

  QDoubleSpinBox *accuracySpinbox = new QDoubleSpinBox();
	accuracySpinbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	accuracySpinbox->setRange(0.0, 1.0);
	accuracySpinbox->setSingleStep(0.05);
	accuracySpinbox->setValue(1 / _pixelPerMeter);
	mainLayout->addWidget(accuracySpinbox, 0, 1);

  QLabel *threasholdLabel = new QLabel(tr("Compare Threashold (m)"));
	mainLayout->addWidget(threasholdLabel, 1, 0);

  QDoubleSpinBox *threasholdSpinbox = new QDoubleSpinBox();
	threasholdSpinbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	threasholdSpinbox->setRange(0.0, 10.0);
	threasholdSpinbox->setSingleStep(0.1);
	threasholdSpinbox->setValue(_diffThreashold);
	mainLayout->addWidget(threasholdSpinbox, 1, 1);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel);

	connect(buttonBox, &QDialogButtonBox::accepted, settingsDialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, settingsDialog, &QDialog::reject);

	mainLayout->addWidget(buttonBox, 2, 0, 1, 2);

  int  result = settingsDialog->exec();

	if (result == QDialog::Accepted)
	{
    _pixelPerMeter  = 1 / accuracySpinbox->value();
		_diffThreashold = threasholdSpinbox->value();
	}

	delete settingsDialog;

	_mainViewer->getMainView()->setCameraManipulator(manip);

	return result;
}

void  DiffAnalysis::calcuZvaluefromDepth(osg::Image *image, osg::Camera *camera)
{
	// If the tile number is one
  double  left, right, bt, top, nr, fr;

	camera->getProjectionMatrixAsOrtho(left, right, bt, top, nr, fr);

  double        xstep             = 1 / _pixelPerMeter;
  double        ystep             = 1 / _pixelPerMeter;
  osg::Matrixd  inverseViewMatrix = camera->getInverseViewMatrix();

	// Rows
	for (int k = 0; k < _posterHeight; k++)
	{
		// Cols
		for (int l = 0; l < _posterWidth; l++)
		{
      float  depthvalue = ((float *)image->data(l, _posterHeight - 1 - k))[0];
      float  z_eye      = (depthvalue) * (nr - fr) - nr;

      osg::Vec3d  worldpos = osg::Vec3d(l * xstep, (_posterHeight - 1 - k) * ystep, z_eye);

			worldpos = worldpos * inverseViewMatrix;

      ((float *)image->data(l, _posterHeight - 1 - k))[0] = worldpos.z();
		}
	}
}

void  DiffAnalysis::writeWithGDAL(osg::Image *image, string path)
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

  std::string  saveName = path;

	// Create the file and copy raster data
	GDALDataset *poDstDS;
	poDstDS = poDriver->Create(saveName.c_str(), _posterWidth, _posterHeight,
                             1, GDT_Byte, papszOptions);
	poDstDS->RasterIO(GF_Write, 0, 0, _posterWidth, _posterHeight,
                    (char *)(image->data()) + _posterWidth * (_posterHeight - 1),
                    _posterWidth, _posterHeight, GDT_Byte, 1, nullptr,
                    sizeof(char), -_posterWidth * sizeof(char), _posterWidth * _posterHeight * sizeof(char));
	poDstDS->GetRasterBand(1)->SetNoDataValue(0);

	// Origin default to top left
  double  topLeft[2] = { _boundingBox.corner(2)[0], _boundingBox.corner(2)[1] };

	poDstDS->SetProjection(_globalWKT);

	// Affine information
  double  adfGeoTransform[6] = { topLeft[0], 1 / _pixelPerMeter, 0, topLeft[1], 0, -1 / _pixelPerMeter };
	poDstDS->SetGeoTransform(adfGeoTransform);

	GDALClose((GDALDatasetH)poDstDS);
}

osg::Vec3  DiffAnalysis::getTranslatedPoint(int x, int y)
{
  double  adfGeoTransform[6] = {
		_boundingBox.corner(2)[0],
		1 / _pixelPerMeter,
		0,
		_boundingBox.corner(2)[1],
		0,
		1 / _pixelPerMeter
	};
  double  left        = adfGeoTransform[0];
  double  top         = adfGeoTransform[3];
  double  pixelWidth  = adfGeoTransform[1];
  double  pixelHeight = adfGeoTransform[5];
  double  right       = left + _posterWidth * pixelWidth;
  double  bottom      = top - _posterHeight * pixelHeight;

	bottom = _boundingBox.corner(0)[1];

  return osg::Vec3(x * pixelWidth + left, y * pixelHeight + bottom, 0);
}

osg::ref_ptr<osg::Image>  DiffAnalysis::analyzeContour(osg::Image *image)
{
	// Use a sobel filter to mark non-contour points as 0
  osg::ref_ptr<osg::Image>  contourImg = new osg::Image;
	contourImg->allocateImage(_posterWidth, _posterHeight, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE);

	for (int i = _posterHeight - 2; i >= 1; i--)
	{
		for (int j = 1; j < _posterWidth - 1; j++)
		{
      float  Gx  = applyCoref(image, coreX, i, j);
      float  Gy  = applyCoref(image, coreY, i, j);
      int    G_2 = (int)(Gx + 0.5) + (int)(Gy + 0.5);
      *(char *)contourImg->data(j, i) = G_2 == 0 ? 0 : 1;
		}
	}

	for (int i = _posterHeight - 1; i >= 0; i--)
	{
    *(char *)contourImg->data(0, i)                = 0;
    *(char *)contourImg->data(_posterWidth - 1, i) = 0;
	}

	for (int j = 0; j < _posterWidth; j++)
	{
    *(char *)contourImg->data(j, 0)                 = 0;
    *(char *)contourImg->data(j, _posterHeight - 1) = 0;
	}

	writeWithGDAL(contourImg, "./rawContour.tiff");

	skeletonize(contourImg);

  return contourImg;
}

void  DiffAnalysis::skeletonize(osg::Image *image)
{
	// algorithm from https://github.com/linbojin/Skeletonization-by-Zhang-Suen-Thinning-Algorithm

	// Do until converged
  std::vector<std::pair<int, int>>  pointsToSet;
  bool                              converged = false;

  while (!converged)
  {
		converged = true;

		// Step 1
		for (int i = _posterHeight - 2; i >= 1; i--)
		{
			for (int j = 1; j < _posterWidth - 1; j++)
			{
				if (shouldSet(image, i, j, 0))
        {
          pointsToSet.push_back(std::pair<int, int>(i, j));
        }
      }
		}

		if (!pointsToSet.empty())
		{
			converged = false;

      for (auto &coord : pointsToSet)
			{
        *(char *)image->data(coord.second, coord.first) = 0;
			}

			cout << "Set " << pointsToSet.size() << " points in step 1" << endl;
			pointsToSet.clear();
		}

		// Step 2
		for (int i = _posterHeight - 2; i >= 1; i--)
		{
			for (int j = 1; j < _posterWidth - 1; j++)
			{
				if (shouldSet(image, i, j, 1))
        {
          pointsToSet.push_back(std::pair<int, int>(i, j));
        }
      }
		}

		if (!pointsToSet.empty())
		{
			converged = false;

      for (auto &coord : pointsToSet)
			{
        *(char *)image->data(coord.second, coord.first) = 0;
			}

			cout << "Set " << pointsToSet.size() << " points in step 2" << endl;
			pointsToSet.clear();
		}
	}
}
