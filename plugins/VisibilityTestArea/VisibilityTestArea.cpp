#include "VisibilityTestArea.h"

#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QAction>

#include <osg/LineWidth>
#include <osg/ShapeDrawable>
#include <osg/TextureCubeMap>
#include <osg/BoundingSphere>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <osgUtil/SmoothingVisitor>
#include <osgSim/OverlayNode>
#include <osgQt/GraphicsWindowQt>

#include <ViewerWidget/ViewerWidget.h>

static const int   SM_TEXTURE_WIDTH  = 1024;
static const bool  SHOW_DEBUG_CAMERA = false;

class SmoothingCallback: public osgDB::Registry::ReadFileCallback
{
public:
	virtual osgDB::ReaderWriter::ReadResult  readNode(const std::string &fileName, const osgDB::ReaderWriter::Options *options) override
	{
		osgDB::ReaderWriter::ReadResult  result     = osgDB::Registry::instance()->readNodeImplementation(fileName, options);
		osg::Node                       *loadedNode = result.getNode();

		if (loadedNode)
		{
			osgUtil::SmoothingVisitor  smoothing;
			loadedNode->accept(smoothing);
		}

		return result;
	}
};

static osg::Vec4  colorToVec(const QColor &color)
{
	return osg::Vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

static osg::ref_ptr<osg::Program>  generateShader(const QString &vertFile, const QString &fragFile, QString geomFile = "")
{
	osg::ref_ptr<osg::Program>  program = new osg::Program;

	if (!vertFile.isEmpty())
	{
		osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::VERTEX);
		program->addShader(shader);

		if (!shader->loadShaderSourceFromFile(vertFile.toLocal8Bit().toStdString()))
		{
			osg::notify(osg::WARN) << "vertex program load failed" << std::endl;

			return nullptr;
		}
	}

	if (!fragFile.isEmpty())
	{
		osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::FRAGMENT);
		program->addShader(shader);

		if (!shader->loadShaderSourceFromFile(fragFile.toLocal8Bit().toStdString()))
		{
			osg::notify(osg::WARN) << "fragment program load failed" << std::endl;

			return nullptr;
		}
	}

	if (!geomFile.isEmpty())
	{
		osg::ref_ptr<osg::Shader>  shader = new osg::Shader(osg::Shader::GEOMETRY);
		program->addShader(shader);

		if (!shader->loadShaderSourceFromFile(geomFile.toLocal8Bit().toStdString()))
		{
			osg::notify(osg::WARN) << "geometry program load failed" << std::endl;

			return nullptr;
		}
	}

	return program;
}

VisibilityTestArea::VisibilityTestArea():
	_userRadius(200),
	_userHeight(3),
	_attributePanel(NULL)
{
	_pluginName     = tr("Visibility Test Area");
	_pluginCategory = "Analysis";
}

VisibilityTestArea::~VisibilityTestArea()
{
}

void  VisibilityTestArea::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("visibilityTestAreaAction"));
	_action->setCheckable(true);
	QIcon  icon;
	icon.addFile(QStringLiteral("resources/icons/visibility.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon);
	_action->setText(tr("Area Visibility"));
	_action->setToolTip(tr("Visibility Test over an Area"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	QToolButton *groupButton = toolBar->findChild<QToolButton *>("VisibilityAnalysisGroupButton", Qt::FindDirectChildrenOnly);

	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("VisibilityAnalysisGroupButton");
		QIcon  icon1;
		icon1.addFile(QString::fromUtf8("resources/icons/vision.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(icon1);
		groupButton->setPopupMode(QToolButton::InstantPopup);
		groupButton->setCheckable(true);
		// groupButton->setDefaultAction(_action);
		groupButton->setText(tr("Visibility"));
		groupButton->setToolTip(tr("Visibility Analysis"));
		QMenu *drawLineMenu = new QMenu(groupButton);
		drawLineMenu->setTitle(tr("Visibility Analysis"));
		drawLineMenu->setIcon(icon1);
		drawLineMenu->addAction(_action);
		groupButton->setMenu(drawLineMenu);
		toolBar->addWidget(groupButton);

		menu->addMenu(drawLineMenu);
	}
	else
	{
		groupButton->menu()->addAction(_action);
	}

	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

osg::Geode* makeIndicator(osg::Vec3 eye)
{
	osg::ref_ptr<osg::Sphere>         pSphereShape   = new osg::Sphere(eye, 1.0f);
	osg::ref_ptr<osg::ShapeDrawable>  pShapeDrawable = new osg::ShapeDrawable(pSphereShape.get());

	pShapeDrawable->setColor(osg::Vec4(1.0, 1.0, 1.0, 1.0));

	osg::ref_ptr<osg::Geode>  geode = new osg::Geode();
	geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	geode->getOrCreateStateSet()->setAttribute(new osg::LineWidth(1.0), osg::StateAttribute::ON);

	geode->addDrawable(pShapeDrawable.get());

	return geode.release();
}

void  VisibilityTestArea::showControlPanel()
{
	if (!_attributePanel)
	{
		_attributeDock = new QDockWidget(_mainWindow);
		_attributeDock->setFeatures(QDockWidget::DockWidgetMovable);
		_attributeDock->setAllowedAreas(NULL);
		_attributeDock->setFloating(true);

		QPoint  viewerPos  = _mainViewer->pos();
		QSize   viewerSize = _mainViewer->size();
		QPoint  newPos(viewerPos.rx() + viewerSize.width() - 300 - 50, 150);
		_attributeDock->setGeometry(newPos.rx(), newPos.ry(), 300, 100);

		_attributeDock->setWindowOpacity(0.9);
		_attributeDock->setWindowFlags(_attributeDock->windowFlags() | Qt::FramelessWindowHint);

		_attributePanel = new QWidget(_attributeDock);

		QGridLayout *mainLayout = new QGridLayout();
		mainLayout->setSpacing(10);

		QLabel  *heightLabel  = new QLabel(tr("Source height:"));
		QLabel  *radiusLabel  = new QLabel(tr("Effect radius:"));
		QSlider *heightSlider = new QSlider(Qt::Horizontal);
		heightSlider->setSingleStep(1);
		heightSlider->setPageStep(10);
		heightSlider->setRange(0, 100);
		heightSlider->setValue(_userHeight);
		heightSlider->setMinimumWidth(100);

		QSlider *radiusSlider = new QSlider(Qt::Horizontal);
		radiusSlider->setSingleStep(50);
		radiusSlider->setPageStep(500);
		radiusSlider->setRange(50, 500);
		radiusSlider->setValue(_userRadius);
		radiusSlider->setMinimumWidth(100);

		_heightBox = new QSpinBox();
		_heightBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_heightBox->setSingleStep(1);
		_heightBox->setRange(0, 100);
		_heightBox->setValue(_userHeight);

		_radiusBox = new QSpinBox();
		_radiusBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
		_radiusBox->setSingleStep(50);
		_radiusBox->setRange(50, 500);
		_radiusBox->setValue(_userRadius);

		_okButton = new QPushButton(tr("OK"));
		_okButton->setEnabled(true);

		mainLayout->addWidget(heightLabel, 0, 0, Qt::AlignLeft);
		mainLayout->addWidget(radiusLabel, 1, 0, Qt::AlignLeft);

		mainLayout->addWidget(heightSlider, 0, 1, Qt::AlignCenter);
		mainLayout->addWidget(radiusSlider, 1, 1, Qt::AlignCenter);

		mainLayout->addWidget(_heightBox, 0, 2, Qt::AlignCenter);
		mainLayout->addWidget(_radiusBox, 1, 2, Qt::AlignCenter);

		mainLayout->addWidget(_okButton, 2, 2, Qt::AlignRight);

		_attributePanel->setLayout(mainLayout);

		connect(_heightBox, SIGNAL(valueChanged(int)), heightSlider, SLOT(setValue(int)));
		connect(heightSlider, SIGNAL(valueChanged(int)), this, SLOT(heightSliderChanged(int)));
		connect(_radiusBox, SIGNAL(valueChanged(int)), radiusSlider, SLOT(setValue(int)));
		connect(radiusSlider, SIGNAL(valueChanged(int)), this, SLOT(radiusSliderChanged(int)));

		connect(_okButton, SIGNAL(clicked()), this, SLOT(setAttributes()));

		_attributeDock->setWidget(_attributePanel);
	}

	_attributeDock->setVisible(true);
}

void  VisibilityTestArea::generateTestSphere(osg::ref_ptr<osg::TextureCubeMap> depthMap, osg::ref_ptr<osg::TextureCubeMap> colorMap)
{
	osg::ref_ptr<osg::Program>  depthVisualizer = generateShader(
		"resources/shaders/depthVisualizer.vert",
		"resources/shaders/depthVisualizer.frag");

	debugNode = new osg::PositionAttitudeTransform;
	debugNode->setPosition(_anchoredOffset);
	debugNode->setCullingActive(false);

	osg::StateSet *ss = debugNode->getOrCreateStateSet();

	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	ss->setTextureAttributeAndModes(0, depthMap, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->setTextureAttributeAndModes(1, colorMap, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->setAttribute(depthVisualizer, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->addUniform(new osg::Uniform("center", _currentWorldPos + osg::Vec3 { 0, 0, 70 }), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->addUniform(new osg::Uniform("depthMap", 0), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	ss->addUniform(new osg::Uniform("colorMap", 1), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

	osg::ref_ptr<osg::TessellationHints>  tsHint = new osg::TessellationHints;
	tsHint->setDetailRatio(8.0);
	osg::ref_ptr<osg::Geode>  geode = new osg::Geode;
	geode->addDrawable(new osg::ShapeDrawable(new osg::Sphere(_anchoredWorldPos + osg::Vec3 { 0, 0, 70 }, 50), tsHint));

	debugNode->addChild(geode);
	_parentScene->addChild(debugNode);
}

osg::Camera * VisibilityTestArea::generateCubeCamera(osg::ref_ptr<osg::TextureCubeMap> cubeMap, unsigned face, osg::Camera::BufferComponent component)
{
	osg::ref_ptr<osg::Camera>  camera = new osg::Camera;

	camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setRenderOrder(osg::Camera::PRE_RENDER);
	camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	camera->setViewport(0, 0, SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
	camera->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
	camera->attach(component, cubeMap, 0, face);

	camera->setNodeMask(0xffffffff & (~INTERSECT_IGNORE));
	camera->addChild(_shadowedScene);
	_parentScene->addChild(camera);

	return camera.release();
}

void  VisibilityTestArea::heightSliderChanged(int value)
{
	if (value != _heightBox->value())
	{
		_heightBox->setValue(value);
	}
}

void  VisibilityTestArea::radiusSliderChanged(int value)
{
	if (value != _radiusBox->value())
	{
		_radiusBox->setValue(value);
	}
}

void  VisibilityTestArea::setAttributes()
{
	_userHeight = _heightBox->value();
	_userRadius = _radiusBox->value();

	_okButton->setEnabled(false);
	updateAttributes();
	_okButton->setEnabled(true);
}

void  VisibilityTestArea::updateAttributes()
{
	// Light source info
	osg::Vec3  lightPos = _pickedPos;

	lightPos.z() += _userHeight;

	// Light source in scene
	if (_currentDrawNode.valid())
	{
		_currentAnchor->removeChild(_currentDrawNode);
	}

	_currentDrawNode = makeIndicator(lightPos - _anchoredOffset);
	_currentAnchor->addChild(_currentDrawNode);

	// Light source in shader
	float                     near_plane = 0.1f;
	float                     far_plane  = 500.0f;
	osg::Matrix               shadowProj = osg::Matrix::perspective(90.0, SM_TEXTURE_WIDTH / SM_TEXTURE_WIDTH, near_plane, far_plane);
	std::vector<osg::Matrix>  shadowViews;
	shadowViews.push_back(
		osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(1.0, 0.0, 0.0), osg::Vec3(0.0, -1.0, 0.0)));
	shadowViews.push_back(
		osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(-1.0, 0.0, 0.0), osg::Vec3(0.0, -1.0, 0.0)));
	shadowViews.push_back(
		osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, 1.0, 0.0), osg::Vec3(0.0, 0.0, 1.0)));
	shadowViews.push_back(
		osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, -1.0, 0.0), osg::Vec3(0.0, 0.0, -1.0)));
	shadowViews.push_back(
		osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, 0.0, 1.0), osg::Vec3(0.0, -1.0, 0.0)));
	shadowViews.push_back(
		osg::Matrix::lookAt(lightPos, lightPos + osg::Vec3(0.0, 0.0, -1.0), osg::Vec3(0.0, -1.0, 0.0)));

	// Update light source info for shadow map
	for (int i = 0; i < 6; i++)
	{
		auto  depthCamera = _depthCameras[i];
		depthCamera->setViewMatrix(shadowViews[i]);
		depthCamera->setProjectionMatrix(shadowProj);
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("lightPos", lightPos));
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("far_plane", far_plane));
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("near_plane", near_plane));
		depthCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("inverse_view", osg::Matrixf::inverse(shadowViews[i])));

		if (SHOW_DEBUG_CAMERA)
		{
			auto  colorCamera = _colorCameras[i];
			colorCamera->setViewMatrix(shadowViews[i]);
			colorCamera->setProjectionMatrix(shadowProj);
		}
	}

	// Update light source info for shadowing scene
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("lightPos", lightPos));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("far_plane", far_plane));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("near_plane", near_plane));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("user_area", (float)_userRadius));
}

void  VisibilityTestArea::toggle(bool checked /*= true*/)
{
	if (!checked)
	{
		onRightButton();
	}

	PluginInterface::toggle(checked);
}

void  VisibilityTestArea::onLeftButton()
{
	// If shadow exist
	if (_shadowedScene.valid() || _movingMode)
	{
		return;
	}

	beginDrawing();
	showControlPanel();

	// Parameters
	_shadowedScene = _dataRoot;
	_parentScene   = _dataRoot->getParent(0);

	_mainViewer->getMainContext()->getState()->setUseModelViewAndProjectionUniforms(true);
	_pickedPos = _currentWorldPos;

	// Keep normals updated
	osgUtil::SmoothingVisitor  smoothing;
	_shadowedScene->accept(smoothing);
	osgDB::Registry::instance()->setReadFileCallback(new SmoothingCallback);

	// Generate a shadow map
	osg::ref_ptr<osg::TextureCubeMap>  depthMap = new osg::TextureCubeMap;
	depthMap->setTextureSize(SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
	depthMap->setInternalFormat(GL_DEPTH_COMPONENT);
	depthMap->setSourceFormat(GL_DEPTH_COMPONENT);
	depthMap->setSourceType(GL_FLOAT);
	depthMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
	depthMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
	depthMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
	depthMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
	depthMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

	// Depth shader that writes unnormaized depth into buffer
	osg::ref_ptr<osg::Program>  depthShader = generateShader(
		"resources/shaders/depthMap.vert", "resources/shaders/depthMap.frag");

	// Generate one camera for each side of the shadow cubemap
	for (int i = 0; i < 6; i++)
	{
		_depthCameras[i] = generateCubeCamera(depthMap, i, osg::Camera::DEPTH_BUFFER);
		_depthCameras[i]->getOrCreateStateSet()->setAttribute(depthShader, osg::StateAttribute::ON);
	}

	if (SHOW_DEBUG_CAMERA)
	{
		osg::ref_ptr<osg::TextureCubeMap>  colorMap = new osg::TextureCubeMap;
		colorMap->setTextureSize(SM_TEXTURE_WIDTH, SM_TEXTURE_WIDTH);
		colorMap->setInternalFormat(GL_RGB);
		colorMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
		colorMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
		colorMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
		colorMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
		colorMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

		for (int i = 0; i < 6; i++)
		{
			_colorCameras[i] = generateCubeCamera(colorMap, i, osg::Camera::COLOR_BUFFER);
		}

		generateTestSphere(depthMap, colorMap);
	}

	// Render the result in shader
	_renderProgram = generateShader("resources/shaders/visibilityShader.vert", "resources/shaders/visibilityShader.frag");

	if (!_renderProgram.valid())
	{
		onRightButton();

		return;
	}

	_parentScene->getOrCreateStateSet()->setTextureAttributeAndModes(1, depthMap, osg::StateAttribute::ON);
	_parentScene->getOrCreateStateSet()->setAttribute(_renderProgram, osg::StateAttribute::ON);
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", 0));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("shadowMap", 1));

	QColor  visibleColor   = getOrAddPluginSettings("Visible color", QColor { 159, 255, 61 }).value<QColor>();
	QColor  invisibleColor = getOrAddPluginSettings("Invisible color", QColor { 255, 87, 61 }).value<QColor>();
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("visibleColor", colorToVec(visibleColor)));
	_parentScene->getOrCreateStateSet()->addUniform(new osg::Uniform("invisibleColor", colorToVec(invisibleColor)));

	updateAttributes();
	_movingMode = true;
}

void  VisibilityTestArea::onRightButton()
{
	if (!_activated || !_shadowedScene.valid())
	{
		return;
	}

	if (!_movingMode)
	{
		_movingMode = true;

		return;
	}

	endDrawing();
	_currentAnchor->removeChild(_currentDrawNode);
	_currentDrawNode = NULL;

	for (auto camera : _depthCameras)
	{
		_parentScene->removeChild(camera);
		camera = NULL;
	}

	if (SHOW_DEBUG_CAMERA)
	{
		for (auto camera : _colorCameras)
		{
			_parentScene->removeChild(camera);
			camera = NULL;
		}

		_parentScene->removeChild(debugNode);
	}

	_parentScene->getOrCreateStateSet()->setAttribute(_renderProgram, osg::StateAttribute::OFF);
	_parentScene->getOrCreateStateSet()->removeAttribute(osg::StateAttribute::PROGRAM);
	_parentScene->getOrCreateStateSet()->removeTextureAttribute(1, osg::StateAttribute::TEXTURE);

	osgDB::Registry::instance()->setReadFileCallback(NULL);

	_mainViewer->getMainContext()->getState()->setUseModelViewAndProjectionUniforms(false);

	_attributeDock->hide();

	_shadowedScene = NULL;

	_movingMode = false;
}

void  VisibilityTestArea::onMouseMove()
{
	if (_isDrawing && _movingMode)
	{
		_pickedPos = _currentWorldPos;
		updateAttributes();
	}
}

void  VisibilityTestArea::onDoubleClick()
{
	if (_isDrawing)
	{
		_movingMode = false;
	}
}
