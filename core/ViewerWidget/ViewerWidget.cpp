#include <ViewerWidget/ViewerWidget.h>

#include <QGridLayout>

#include <osg/CullFace>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/AutoTransform>
#include <osg/BlendFunc>
#include <osgDB/ReadFile>
#include <osgText/Text>
#include <osgViewer/ViewerEventHandlers>
#include <osgQt/GraphicsWindowQt>
#include <osgGA/StateSetManipulator>

#include <osgEarth/GLUtils>

#include <DataManager/FindNode.hpp>

#include "Compass.h"

static const int  DEFAULT_FRAME_RATE = 60;

ViewerWidget::ViewerWidget(osg::Node *mainScene, int x, int y, int w, int h,
                           osgViewer::ViewerBase::ThreadingModel threadingModel /*=osgViewer::CompositeViewer::SingleThreaded*/):
	QWidget()
{
	setThreadingModel(threadingModel);
	setKeyEventSetsDone(0);

	setRunMaxFrameRate(DEFAULT_FRAME_RATE);

	// Turn off all lights by default
	osg::StateSet *state = mainScene->getOrCreateStateSet();
	state->setMode(GL_LIGHTING, osg::StateAttribute::OFF &osg::StateAttribute::OVERRIDE);
	state->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

	osg::ref_ptr<osg::CullFace>  cf = new osg::CullFace;
	cf->setMode(osg::CullFace::BACK);
	state->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
	state->setAttributeAndModes(cf, osg::StateAttribute::ON);

	// Init a main view
	_mainContext = createGraphicsWindow(x, y, w, h);
	_mainWidget  = createViewWidget(_mainContext, mainScene);
	_mainView    = getView(0);

	// Grid layout that contain multiple views
	_grid = new QGridLayout;
	_grid->addWidget(_mainWidget, 0, 0);
	_grid->setMargin(1);
	setLayout(_grid);

	initCompass(mainScene->asGroup());

	// Playback controlling
	_frameRate = DEFAULT_FRAME_RATE;
	connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));
	startRendering();
}

ViewerWidget::~ViewerWidget()
{
}

void  ViewerWidget::setWidgetInLayout(QWidget *widget, int row, int column, bool visible /*= true*/)
{
	_grid->addWidget(widget, row, column);
	widget->setVisible(visible);
}

void  ViewerWidget::removeView(osgViewer::View *view)
{
	stopRendering();
	((QGridLayout *)layout())->removeWidget(_widgets[view]);
	_widgets.remove(view);
	osgViewer::CompositeViewer::removeView(view);
	startRendering();
}

QWidget * ViewerWidget::createViewWidget(osgQt::GraphicsWindowQt *gw, osg::Node *scene)
{
	osg::ref_ptr<osgViewer::View>       view   = new osgViewer::View;
	osg::Camera                        *camera = view->getCamera();
	const osg::GraphicsContext::Traits *traits = gw->getTraits();

	// Connect and align the camera with the given graphics window
	camera->setGraphicsContext(gw);
	camera->setClearColor(osg::Vec4(0.95, 0.95, 0.95, 1.0));
	camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
	camera->setSmallFeatureCullingPixelSize(-1.0f);

	// Init the scene
	osgEarth::GLUtils::setGlobalDefaults(camera->getOrCreateStateSet());

	view->setSceneData(scene);
	view->addEventHandler(new osgViewer::StatsHandler);
	// view->addEventHandler(new osgViewer::ThreadingHandler);
	view->addEventHandler(new osgViewer::WindowSizeHandler);
	view->addEventHandler(new osgViewer::LODScaleHandler);
	// view->addEventHandler(new osgGA::StateSetManipulator(view->getCamera()->getOrCreateStateSet()));

	// Tell the database pager to not modify the unref settings
	view->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);

	// We have to pause all threads before a view will be added to the composite viewer
	stopThreading();
	addView(view);
	startThreading();

	_widgets.insert(view, gw->getGLWidget());

	return gw->getGLWidget();
}

osgQt::GraphicsWindowQt * ViewerWidget::createGraphicsWindow(int x, int y, int w, int h, const std::string &name /*=""*/, bool shareMainContext /*= false*/,
                                                             bool windowDecoration /*=false */)
{
	// Settings of the rendering context
	osg::DisplaySettings *ds = osg::DisplaySettings::instance().get();

	ds->setNumMultiSamples(4);
	osg::ref_ptr<osg::GraphicsContext::Traits>  traits = new osg::GraphicsContext::Traits;
	traits->windowName       = name;
	traits->windowDecoration = windowDecoration;
	traits->x                = x;
	traits->y                = y;
	traits->width            = w;
	traits->height           = h;
	traits->doubleBuffer     = true;
	traits->alpha            = ds->getMinimumNumAlphaBits();
	traits->stencil          = ds->getMinimumNumStencilBits();
	traits->sampleBuffers    = ds->getMultiSamples();
	traits->samples          = ds->getNumMultiSamples();

	// This setting helps to make sure two synced widget live and die together
	if (shareMainContext)
	{
		traits->sharedContext = _mainContext;
	}

	osg::ref_ptr<osgQt::GraphicsWindowQt>  gw = new osgQt::GraphicsWindowQt(traits.get());

	return gw.release();
}

osgViewer::View * ViewerWidget::getMainView()
{
	return _mainView;
}

osgQt::GraphicsWindowQt * ViewerWidget::getMainContext()
{
	return _mainContext;
}

void  ViewerWidget::paintEvent(QPaintEvent *event)
{
	frame();
}

void  ViewerWidget::setMouseStyle(unsigned styleshape)
{
	_mainContext->setCursor((osgViewer::GraphicsWindow::MouseCursor)styleshape);
}

void  ViewerWidget::stopRendering()
{
	_timer.stop();
	stopThreading();
}

void  ViewerWidget::startRendering()
{
	startThreading();
	_timer.start(1000 / _frameRate);
}

void  ViewerWidget::setFrameRate(int FPS)
{
	stopRendering();
	_frameRate = FPS;
	startRendering();
}

osg::ref_ptr<osg::Camera>  ViewerWidget::createLegendHud(const QString &titleString, QVector<osg::Vec4> colorVec, QVector<QString> txtVec)
{
	// An hud camera
	osg::ref_ptr<osg::Camera>  hudCamera = new osg::Camera;

	hudCamera->setName("hudCamera");
	hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0, 1280, 0, 800));
	hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	hudCamera->setViewMatrix(osg::Matrix::identity());
	hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
	hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
	hudCamera->setAllowEventFocus(false);

	// A geode for rendering texts
	osg::ref_ptr<osg::Geode>     pGeode    = new osg::Geode;
	osg::ref_ptr<osg::StateSet>  pStateSet = pGeode->getOrCreateStateSet();
	pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	pStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

	// Generate title
	osg::ref_ptr<osgText::Font>  pFont = new osgText::Font;
	pFont = osgText::readFontFile("simhei.ttf");
	osg::ref_ptr<osgText::Text>  titleText = new osgText::Text;
	titleText->setFont(pFont);
	titleText->setText(titleString.toStdString(), osgText::String::ENCODING_UTF8);
	titleText->setPosition(osg::Vec3(10.0f, 752.0f + 20, -1));
	titleText->setCharacterSize(16.0f);
	titleText->setColor(osg::Vec4(199, 77, 15, 1));
	pGeode->addDrawable(titleText);

	// Color contents
	int  length = colorVec.size();

	for (int i = 0; i < length; i++)
	{
		// For every color, generate a squad with that color as filling color
		osg::ref_ptr<osg::Geometry>  pGeo = new osg::Geometry;
		pGeo = osg::createTexturedQuadGeometry(osg::Vec3(10, 750.0f - (17 * i), -1), osg::Vec3(38, 0.0, 0.0), osg::Vec3(0.0, 15.0, 0.0));

		osg::ref_ptr<osg::Vec4Array>  colorArray = new osg::Vec4Array;
		colorArray->push_back(colorVec.at(i));
		pGeo->setColorArray(colorArray.get());
		pGeo->setColorBinding(osg::Geometry::BIND_OVERALL);

		pGeode->addDrawable(pGeo);

		// Generate the associated describing text
		osg::ref_ptr<osgText::Text>  pText = new osgText::Text;
		pText->setFont(pFont);
		pText->setText(txtVec.at(i).toStdString());
		pText->setPosition(osg::Vec3(52.0f, 752.0f - (17 * i), -1));
		pText->setCharacterSize(15.0f);
		pText->setColor(osg::Vec4(199, 77, 15, 1));

		pGeode->addDrawable(pText);
	}

	hudCamera->addChild(pGeode);

	return hudCamera;
}

osg::ref_ptr<osg::PositionAttitudeTransform>  ViewerWidget::createCameraIndicator()
{
	// Init a transform that always faces camera
	osg::ref_ptr<osg::PositionAttitudeTransform>  cameraIndicator = new osg::PositionAttitudeTransform();
	osg::ref_ptr<osg::AutoTransform>              cameraCenter    = new osg::AutoTransform();

	cameraCenter->setAutoScaleToScreen(true);
	cameraCenter->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
	osg::StateSet *state = cameraCenter->getOrCreateStateSet();
	state->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
	state->setMode(GL_LIGHTING, osg::StateAttribute::OFF &osg::StateAttribute::OVERRIDE);
	state->setRenderBinDetails(99, "RenderBin");

	// Render the image
	osg::ref_ptr<osg::Geode>      geode   = new osg::Geode;
	osg::ref_ptr<osg::Geometry>   geom    = createTexturedQuadGeometry(osg::Vec3(-20, -20, 0), osg::Vec3(40, 0, 0), osg::Vec3(0, 40, 0));
	osg::ref_ptr<osg::Image>      image   = osgDB::readImageFile("resources/icons/center.png");
	osg::ref_ptr<osg::Texture2D>  texture = new osg::Texture2D;
	texture->setTextureSize(100, 100);
	texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
	texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
	texture->setImage(image);
	geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
	geom->getOrCreateStateSet()->setAttributeAndModes(
		new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));

	// Attach to the main camera
	geode->addDrawable(geom);
	cameraCenter->addChild(geode);
	cameraIndicator->addChild(cameraCenter);
	cameraIndicator->setNodeMask(0);

	_mainView->getCamera()->addChild(cameraIndicator);

	return cameraIndicator;
}

void  ViewerWidget::initCompass(osg::Group *root)
{
	float                     radius = 1.5f;
	float                     height = -1.0f;
	osg::Vec3                 center(-radius, -radius, height);
	osg::ref_ptr<osg::Geode>  geode = new osg::Geode;

	geode->addDrawable(
		createTexturedQuadGeometry(center, osg::Vec3(radius * 2.0f, 0.0f, 0.0f), osg::Vec3(0.0f, radius * 2.0f, 0.0f)));

	// Load compass
	osg::ref_ptr<osg::Texture2D>  texture = new osg::Texture2D;
	texture->setImage(osgDB::readImageFile("resources/icons/compass.png"));

	osg::ref_ptr<osg::MatrixTransform>  plate = new osg::MatrixTransform;
	plate->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture.get());
	plate->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	plate->addChild(geode.get());

	osg::ref_ptr<Compass>  compass = new Compass;
	compass->setViewport(0.0, 0.0, 50.0, 50.0);
	compass->setProjectionMatrix(osg::Matrixd::ortho(-1.5, 1.5, -1.5, 1.5, -10.0, 10.0));

	compass->setPlate(plate);
	compass->setMainCamera(getMainView()->getCamera());

	// Render as hud
	compass->setRenderOrder(osg::Camera::POST_RENDER);
	compass->setClearMask(GL_DEPTH_BUFFER_BIT);
	compass->setAllowEventFocus(false);
	compass->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	compass->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	compass->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);

	root->addChild(compass.get());
}
