#include "DrawSurfacePolygon.h"


#include <limits>
using namespace std;

#include <osg/PositionAttitudeTransform>
#include <osgSim/OverlayNode>
#include <osgUtil/Tessellator>

#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QToolButton>

QMap<int, osg::PositionAttitudeTransform *>  DrawSurfacePolygon::_polygs;
int                                          DrawSurfacePolygon::_numSubDraw;

DrawSurfacePolygon::DrawSurfacePolygon():
  _Ymin((numeric_limits<double>::max)())
{
  _pluginName     = tr("Surface Polygon");
	_pluginCategory = "Draw";

	_overlayAlpha = getOrAddPluginSettings("Overlay transparency", 0.92).toDouble();

  _state = new osg::StateSet;
  _state->setMode(GL_BLEND, osg::StateAttribute::ON);
	_state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
}

DrawSurfacePolygon::~DrawSurfacePolygon()
{
}

void  DrawSurfacePolygon::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("drawPlgSrfAction"));
	_action->setCheckable(true);
  QIcon  icon14;
	icon14.addFile(QStringLiteral("resources/icons/drawpolygon.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon14);
	_action->setText(tr("Surface Polygon"));
	_action->setToolTip(tr("Draw Surface Polygon"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

  QToolButton *groupButton = toolBar->findChild<QToolButton *>("DrawPolygonGroupButton", Qt::FindDirectChildrenOnly);

	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("DrawPolygonGroupButton");
    QIcon  icon2;
		icon2.addFile(QString::fromUtf8("resources/icons/drawpolygon.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(icon2);
		groupButton->setPopupMode(QToolButton::MenuButtonPopup);
		groupButton->setCheckable(true);
    // groupButton->setDefaultAction(_action);
		groupButton->setText(tr("Polygon"));
    QMenu *drawPlgMenu = new QMenu(groupButton);
		drawPlgMenu->setTitle(tr("Polygon"));
		drawPlgMenu->setIcon(icon2);
		drawPlgMenu->addAction(_action);
		groupButton->setMenu(drawPlgMenu);
		groupButton->setToolTip(tr("Draw Polygons"));
		toolBar->addWidget(groupButton);

		menu->addMenu(drawPlgMenu);
	}
	else
	{
		groupButton->menu()->addAction(_action);
	}

	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void  DrawSurfacePolygon::onLeftButton()
{
	if (!_isDrawing)
	{
		beginDrawing();
		_startPoint = _anchoredWorldPos;
    _lastPoint  = _startPoint;

		_currentDrawNode = new osg::Geode;
    _zOffsetNode     = new osg::PositionAttitudeTransform;
    _zOffsetNode->setPosition({ 0, 0, _zOffset });
    _zOffsetNode->addChild(_currentDrawNode);

		_contourPoints = new osg::Vec3Array;
    _center        = osg::Vec3(0.0, 0.0, 0.0);

		_lastLine = newLine();
		_currentDrawNode->addDrawable(_lastLine);

    _currentAnchor->addChild(_zOffsetNode);
  }
	else
	{
		_lastPoint = _anchoredWorldPos;
	}

	_lineGeom = newLine();
	_currentDrawNode->addDrawable(_lineGeom);
	_slicer.setStartPoint(_lastPoint + _anchoredOffset);

	_currentDrawNode->addDrawable(createPointGeode(_lastPoint, _intersections.begin()->getLocalIntersectNormal()));

  _contourPoints->push_back({ _lastPoint.x(), _lastPoint.y(), SUBGRAPH_HEIGHT });
	_center += _lastPoint;

	if (_lastPoint.y() < _Ymin)
  {
    _Ymin = _lastPoint.y();
  }
}

void  DrawSurfacePolygon::onRightButton()
{
	if (_isDrawing)
	{
    DrawSurfaceLine::onRightButton();
    _state         = nullptr;
    _lastLine      = nullptr;
    _contourPoints = nullptr;
	}
}

void  DrawSurfacePolygon::onDoubleClick()
{
	if (_isDrawing)
	{
    drawOverlay();
    onRightButton();
	}
}

void  DrawSurfacePolygon::onMouseMove()
{
  if (_isDrawing)
	{
		_endPoint = _anchoredWorldPos;

		if (_lastPoint != _endPoint)
		{
			_slicer.setEndPoint(_endPoint + _anchoredOffset);
			updateIntersectedLine();

      // Enclosing the polygon
      osg::ref_ptr<osg::Geometry>  tempGeom = _lineGeom;
			_lineGeom = _lastLine;
			_slicer.setStartPoint(_startPoint + _anchoredOffset);
			updateIntersectedLine();
			_slicer.setStartPoint(_lastPoint + _anchoredOffset);
			_lineGeom = tempGeom;
		}
	}
}

void  DrawSurfacePolygon::drawOverlay()
{
  // Generate an overlay
	if (_contourPoints->empty())
  {
    return;
  }

  osg::ref_ptr<osg::Geometry>  polyg   = tesselatedPolygon(_contourPoints);
  osg::ref_ptr<osg::Geode>     overlay = new osg::Geode;
  overlay->addDrawable(polyg);

  _drawnOverlay = new osg::PositionAttitudeTransform;
  _drawnOverlay->setPosition(_anchoredOffset);
  _drawnOverlay->addChild(overlay);

  // Save information locally
  _polygs[_numSubDraw]       = _drawnOverlay;
  _center                   /= _contourPoints->size();
	_centerPoints[_numSubDraw] = _center;

  _drawnOverlay->setUserValue("index", _numSubDraw);
	_pluginRoot->setUserValue("index", _numSubDraw);

  // Add to subgraph
  _subgraph->addChild(_drawnOverlay);

  recordNode(_drawnOverlay);

	_numSubDraw++;

  _overlayNode->setOverlayBaseHeight(_Ymin - 10);
  _overlayNode->dirtyOverlayTexture();
}

osg::ref_ptr<osg::Geometry>  DrawSurfacePolygon::tesselatedPolygon(osg::Vec3Array *polygon)
{
  osg::ref_ptr<osg::Geometry>  polyg = new osg::Geometry;
  polyg->setVertexArray((osg::Vec3Array *)polygon->clone(osg::CopyOp::DEEP_COPY_ARRAYS));
  osg::ref_ptr<osg::Vec4Array>  color = new osg::Vec4Array;
	color->push_back(osg::Vec4(_style.fillColor.r(), _style.fillColor.g(), _style.fillColor.b(), _overlayAlpha));
	polyg->setColorArray(color, osg::Array::BIND_OVERALL);
	polyg->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, polygon->size()));
	polyg->setStateSet(_state);

  osg::ref_ptr<osgUtil::Tessellator>  tessellator = new osgUtil::Tessellator();
	tessellator->setTessellationType(osgUtil::Tessellator::TESS_TYPE_GEOMETRY);
	tessellator->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
	tessellator->setTessellationNormal(osg::Vec3(0, 0, 1.0));
	tessellator->retessellatePolygons(*polyg);

	return polyg;
}
