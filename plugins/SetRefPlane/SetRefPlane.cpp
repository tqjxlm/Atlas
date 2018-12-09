#include "SetRefPlane.h"
#include "MeasureVolumePlaneSetDlg.h"

#include <QAction>

#include <iostream>
using namespace std;

#include <osg/ComputeBoundsVisitor>
#include <osg/PositionAttitudeTransform>
#include <osgText/Text>

osg::ref_ptr<osg::Vec3Array>  SetRefPlane::_vertexBP = NULL;

SetRefPlane::SetRefPlane()
{
  _pluginName     = tr("Reference Plane");
	_pluginCategory = "Measure";

  _planeDone       = false;
  _planeSettingDlg = NULL;
  _vertexBP        = NULL;
}

SetRefPlane::~SetRefPlane(void)
{
}

void  SetRefPlane::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("setRefPlaneAction"));
	_action->setCheckable(true);
  QIcon  icon25;
	icon25.addFile(QStringLiteral("resources/icons/layer.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon25);

	_action->setText(tr("Set Measurement Base Plane"));
	_action->setToolTip(tr("Set a Base Plane for Measurements"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);
}

void  SetRefPlane::onLeftButton()
{
	if (!_planeDone)
	{
		if (!_planePoints.valid())
		{
      _planePoints    = new osg::Vec3Array;
			_buildPlaneNode = new osg::Geode;
      _pluginRoot     = new osg::Group;
			_pluginRoot->addChild(_buildPlaneNode);
			_drawRoot->addChild(_pluginRoot);

      auto  str = string("Set plane");
      _text = createTextGeode(str, _anchoredWorldPos);
			_buildPlaneNode->addDrawable(_text);
		}

		if (_planePoints->size() < 3)
		{
			_buildPlaneNode->addDrawable(createPointGeode(_anchoredWorldPos, _intersections.begin()->getWorldIntersectNormal()));
			_planePoints->push_back(_currentWorldPos);
		}

		if (_planePoints->size() == 3)
		{
			_refPlane.set(_planePoints->at(0), _planePoints->at(1), _planePoints->at(2));
			_refPlane.makeUnitLength();
			_planeDone = true;
			_buildPlaneNode->removeDrawable(_text);

			drawPlane();
		}
	}
}

void  SetRefPlane::onRightButton()
{
	if (!_planeDone)
	{
		_planeDone = false;
		_drawRoot->removeChild(_pluginRoot);
		_planePoints.release();

		return;
	}
}

void  SetRefPlane::onMouseMove()
{
	if (!_planeDone)
	{
		if (_text.valid())
    {
      _text->setPosition(_anchoredWorldPos);
    }
  }
}

void  SetRefPlane::onDoubleClick()
{
}

void  SetRefPlane::createBasicPolygon()
{
  osg::ComputeBoundsVisitor  boundsVisitor;
  osg::ref_ptr<osg::Transform> temp = new osg::Transform;
  temp->addChild(_dataRoot);
	boundsVisitor.apply(*temp);
  osg::BoundingBox  boundingBox = boundsVisitor.getBoundingBox();
  osg::Vec3         triaPos1    = _planePoints->at(0);
  osg::Vec3         triaPos2    = _planePoints->at(1);
  osg::Vec3         triaPos3    = _planePoints->at(2);

  osg::Vec3  recPointA = osg::Vec3(boundingBox.xMin(), boundingBox.yMax(), 0);
  osg::Vec3  recPointB = osg::Vec3(boundingBox.xMax(), boundingBox.yMax(), 0);
  osg::Vec3  recPointC = osg::Vec3(boundingBox.xMax(), boundingBox.yMin(), 0);
  osg::Vec3  recPointD = osg::Vec3(boundingBox.xMin(), boundingBox.yMin(), 0);

  osg::Vec3f  aa        = (triaPos1 - triaPos2);
  osg::Vec3f  bb        = (triaPos3 - triaPos2);
  osg::Vec3f  planeNorm = aa ^ bb;
	planeNorm.normalize();

	if (planeNorm.z() < 0)
  {
    planeNorm *= -1;
  }

  float  dd = triaPos1 * planeNorm;

  recPointA.z() = dd - planeNorm.x() * recPointA.x() - planeNorm.y() * recPointA.y();
  recPointB.z() = dd - planeNorm.x() * recPointB.x() - planeNorm.y() * recPointB.y();
  recPointC.z() = dd - planeNorm.x() * recPointC.x() - planeNorm.y() * recPointC.y();
  recPointD.z() = dd - planeNorm.x() * recPointD.x() - planeNorm.y() * recPointD.y();

  osg::Vec3Array *points = new osg::Vec3Array;
	points->push_back(recPointA);
	points->push_back(recPointB);
	points->push_back(recPointC);
	points->push_back(recPointD);

  osg::ref_ptr<osg::Geometry>  geom = new osg::Geometry;
	geom->setVertexArray(points);

  osg::ref_ptr<osg::Vec4Array>  color = new osg::Vec4Array;
	color->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 0.5f));
	geom->setColorArray(color, osg::Array::BIND_OVERALL);

	geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, points->size()));

  osg::ref_ptr<osg::StateSet>  state = geom->getOrCreateStateSet();
	state->setMode(GL_LIGHTING, osg::StateAttribute::ON);
	state->setMode(GL_BLEND, osg::StateAttribute::ON);
	state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

  osg::PositionAttitudeTransform *transnode = new osg::PositionAttitudeTransform;
  osg::Geode                     *drawnode  = new osg::Geode;
	drawnode->addDrawable(geom);
	transnode->addChild(drawnode);
  osg::Vec3  center = (points->at(0) + points->at(1) + points->at(2)) / 3;
  transnode->setPosition(osg::Matrix::translate(-center) * osg::Matrix::translate(center) * osg::Matrix::translate(-_anchoredOffset).getTrans());
	_pluginRoot->addChild(transnode);
}

void  SetRefPlane::drawPlane()
{
	if (!_planeSettingDlg)
	{
		_planeSettingDlg = new MeasureVolumePlaneSetDlg;
	}

	_planeSettingDlg->setPlaneCoordinateInfo(_planePoints);

	if (_planeSettingDlg->exec())
	{
		_planePoints = _planeSettingDlg->getPlanePoints();
	}
	else
	{
		_planeDone = false;
		_drawRoot->removeChild(_pluginRoot);
		_planePoints.release();

		return;
	}

	createBasicPolygon();

	recordNode(_pluginRoot, "BasicPlane4Measure");

	resetButton();
}

void  SetRefPlane::resetButton()
{
	_vertexBP.release();
	_vertexBP = new osg::Vec3Array;
	_vertexBP->push_back(_planePoints->at(0));
	_vertexBP->push_back(_planePoints->at(1));
	_vertexBP->push_back(_planePoints->at(2));

	_planeDone = false;
	_planePoints.release();
}
