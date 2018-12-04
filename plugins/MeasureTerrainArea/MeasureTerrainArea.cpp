#include "MeasureTerrainArea.h"

#include <iostream>
#include <QIcon>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QToolButton>

#include <osg/TriangleFunctor>
#include <osgUtil/PolytopeIntersector>
#include <osgUtil/DelaunayTriangulator>
#include <osgSim/LineOfSight>
#include <osgSim/OverlayNode>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osgText/Text>

#include "AreaVisitor.h"

inline float  calcTriArea(osg::Vec3 &p1, osg::Vec3 &p2, osg::Vec3 &p3)
{
	return ((p1 - p2) ^ (p1 - p3)).length() / 2;
}

MeasureTerrainArea::MeasureTerrainArea()
{
  _pluginName     = tr("Surface Area");
	_pluginCategory = "Measure";

  QMap<QString, QVariant>  customStyle;
  customStyle["Text color"]    = QColor(255, 253, 61);
  customStyle["Text floating"] = 10.0f;

  getOrAddPluginSettings("Draw style", customStyle);
}

MeasureTerrainArea::~MeasureTerrainArea()
{
}

void  MeasureTerrainArea::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("measSurfAreAction"));
	_action->setCheckable(true);
  QIcon  icon20;
	icon20.addFile(QStringLiteral("resources/icons/area.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon20);
	_action->setText(tr("Surface Area"));
	_action->setToolTip(tr("Measure Surface Area"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

  QToolButton *groupButton = toolBar->findChild<QToolButton *>("MeasureAreaGroupButton", Qt::FindDirectChildrenOnly);

	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("MeasureAreaGroupButton");
    QIcon  iconAre;
		iconAre.addFile(QString::fromUtf8("resources/icons/area.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(iconAre);
		groupButton->setPopupMode(QToolButton::InstantPopup);
		groupButton->setCheckable(true);
    // groupButton->setDefaultAction(_action);
		groupButton->setText(tr("Area"));
		QMenu *drawAreaMenu = new QMenu(groupButton);
		drawAreaMenu->setTitle(tr("Measure area"));
		drawAreaMenu->addAction(_action);
		drawAreaMenu->setIcon(iconAre);
		groupButton->setMenu(drawAreaMenu);
		groupButton->setToolTip(tr("Measure area"));
		toolBar->addWidget(groupButton);

		menu->addMenu(drawAreaMenu);
	}
	else
	{
		groupButton->menu()->addAction(_action);
	}

	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void  MeasureTerrainArea::onDoubleClick()
{
	if (_isDrawing)
	{
    endDrawing();
		drawOverlay();
    _currentDrawNode->removeChildren(0, _currentDrawNode->getNumChildren());
    _lineGeom      = nullptr;
    _state         = nullptr;
    _lastLine      = nullptr;
    _contourPoints = nullptr;

    if (calculateArea() == false)
    {
      onRightButton();
    }
    else
    {
      osg::ref_ptr<osg::Group>  dataGroup = new osg::Group;
      dataGroup->setNodeMask(0x0);
      dataGroup->addChild(_currentDrawNode);
      dataGroup->addChild(_drawnOverlay);
      recordNode(dataGroup);
    }
	}
}

void  MeasureTerrainArea::onLeftButton()
{
	if (!_isDrawing)
  {
    _contour = new osg::Vec3Array;
  }

	_contour->push_back(_currentWorldPos);
	DrawSurfacePolygon::onLeftButton();
}

float  MeasureTerrainArea::areaInBoundary(osg::Node *node, osg::Vec3Array *boundary)
{
  osg::ref_ptr<osg::Vec2Array>  boundary2d = new osg::Vec2Array;

	for (int i = 0; i < boundary->size(); i++)
	{
		boundary2d->push_back(osg::Vec2(boundary->at(i).x(), boundary->at(i).y()));
	}

  AreaVisitor  av(boundary2d, getOrAddPluginSettings("Measure highest level", false).toBool());
	node->accept(av);

	return av.getArea();
}

bool  MeasureTerrainArea::calculateArea()
{
  osg::ref_ptr<osg::Geometry>      polyg = tesselatedPolygon(_contour);
  osg::ref_ptr<osg::Vec3Array>     vertices;
  osg::ref_ptr<osg::PrimitiveSet>  prim;
  float                            area = 0;

	for (unsigned int numPrim = 0; numPrim < polyg->getNumPrimitiveSets(); numPrim++)
	{
    prim     = polyg->getPrimitiveSet(numPrim);
    vertices = (osg::Vec3Array *)polyg->getVertexArray();

		std::cout << "numIndices" << prim->getNumIndices() << std::endl;
		std::cout << "numInstances" << prim->getNumInstances() << std::endl;
		std::cout << "numPrimitives" << prim->getNumPrimitives() << std::endl;

    switch (prim->getMode())
		{
    case (osg::PrimitiveSet::TRIANGLES):

			for (unsigned int i = 0; i < prim->getNumIndices(); i += 3)
			{
        osg::ref_ptr<osg::Vec3Array>  ctrlPoints = new osg::Vec3Array;
				ctrlPoints->push_back(vertices->at(prim->index(i)));
        ctrlPoints->push_back(vertices->at(prim->index(i + 1)));
        ctrlPoints->push_back(vertices->at(prim->index(i + 2)));

				area += areaInBoundary(_overlayNode, ctrlPoints);
			}

			break;
    case (osg::PrimitiveSet::TRIANGLE_STRIP):

			for (unsigned int i = 0; i < prim->getNumIndices() - 2; i++)
			{
        osg::ref_ptr<osg::Vec3Array>  ctrlPoints = new osg::Vec3Array;

				if (i % 2 == 0)
				{
					ctrlPoints->push_back(vertices->at(prim->index(i)));
          ctrlPoints->push_back(vertices->at(prim->index(i + 1)));
          ctrlPoints->push_back(vertices->at(prim->index(i + 2)));
				}
				else
				{
          ctrlPoints->push_back(vertices->at(prim->index(i + 1)));
					ctrlPoints->push_back(vertices->at(prim->index(i)));
          ctrlPoints->push_back(vertices->at(prim->index(i + 2)));
				}

				area += areaInBoundary(_overlayNode, ctrlPoints);
			}

			break;
    case (osg::PrimitiveSet::TRIANGLE_FAN):

			for (unsigned int i = 1; i < prim->getNumIndices() - 1; i++)
			{
        osg::ref_ptr<osg::Vec3Array>  ctrlPoints = new osg::Vec3Array;
				ctrlPoints->push_back(vertices->at(prim->index(0)));
				ctrlPoints->push_back(vertices->at(prim->index(i)));
        ctrlPoints->push_back(vertices->at(prim->index(i + 1)));

				area += areaInBoundary(_overlayNode, ctrlPoints);
			}

			break;
		default:

			return false;
		}
  }

	_currentDrawNode->setUserValue("area", area);

  auto  str = tr("%1m2").arg(area, 0, 'f', 1).toStdString();
  showTxtAtCenter(str);

	return true;
}

void  MeasureTerrainArea::showTxtAtCenter(std::string &txt)
{
  // Get the top point at the center of all contour points
  osg::Vec3  start = _center + _anchoredOffset;
	start.z() = SUBGRAPH_HEIGHT;
  osg::Vec3  end = start;
	end.z() = -SUBGRAPH_HEIGHT;
  osg::ref_ptr<osgUtil::LineSegmentIntersector>  intersector = new osgUtil::LineSegmentIntersector(start, end);
  osgUtil::IntersectionVisitor                   _iv(intersector.get());
	_dataRoot->accept(_iv);

  // Add a text label and a point at the position
  auto  intersection = intersector->getFirstIntersection();
  auto  anchored     = intersection.getWorldIntersectPoint() - _anchoredOffset;

	_currentDrawNode->addDrawable(createTextGeode(txt, anchored));
  _currentDrawNode->addDrawable(createPointGeode(anchored, intersection.getLocalIntersectNormal()));
}

void  MeasureTerrainArea::setBoundingPolytope(osg::Polytope &boundingPolytope, const osg::Vec3Array *ctrlPoints, const osg::Vec3 &up)
{
  int  numIndices = ctrlPoints->size();

	for (int j = 0; j < numIndices; j++)
	{
    osg::Vec3  planeNormal = (ctrlPoints->at(j) - ctrlPoints->at((j + 1) % numIndices)) ^ up;
		planeNormal.normalize();
    osg::Plane  plane;
    plane.set(planeNormal, ctrlPoints->at(j));
    boundingPolytope.add(plane);
	}
}
