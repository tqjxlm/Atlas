#include "MeasureTerrainVolume.h"

#include <QMessageBox>
#include <QIcon>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QToolButton>

#include <osg/Geometry>
#include <osgSim/OverlayNode>
#include <SetRefPlane/SetRefPlane.h>

#include "VolumeVisitor.h"

inline float  calcPyramidVol(const osg::Vec3 &v1, const osg::Vec3 &v2, const osg::Vec3 &v3, const osg::Vec3 &apex)
{
  return ((v3 - v2) ^ (v1 - v2)) * (apex - v2) / 6;
}

MeasureTerrainVolume::MeasureTerrainVolume():
	_planeDone(false)
{
  _pluginName     = tr("Surface Volume");
	_pluginCategory = "Measure";
}

MeasureTerrainVolume::~MeasureTerrainVolume()
{
}

void  MeasureTerrainVolume::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("measTerVolAction"));
	_action->setCheckable(true);
  QIcon  icon22;
	icon22.addFile(QStringLiteral("resources/icons/cube.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon22);
	_action->setText(tr("Surface Volume"));
	_action->setToolTip(tr("Measure Volume under Surface"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

  QToolButton *groupButton = toolBar->findChild<QToolButton *>("MeasureVolumeGroupButton", Qt::FindDirectChildrenOnly);

	if (!groupButton)
	{
		QToolButton *measureVolButton = new QToolButton(_mainWindow);
		measureVolButton->setObjectName("MeasureVolumeGroupButton");
    QIcon  iconBP;
		iconBP.addFile(QString::fromUtf8("resources/icons/cube.png"), QSize(), QIcon::Normal, QIcon::Off);
    // measureVolButton->setDefaultAction(_action);
		measureVolButton->setIcon(iconBP);
		measureVolButton->setPopupMode(QToolButton::InstantPopup);
		measureVolButton->setCheckable(true);
		measureVolButton->setText(tr("Volume"));
		QMenu *measureVolMenu = new QMenu(measureVolButton);
		measureVolMenu->setTitle(tr("Measure Volume"));
		measureVolMenu->setIcon(iconBP);
		measureVolMenu->addAction(_action);
		measureVolButton->setMenu(measureVolMenu);
		measureVolButton->setToolTip(tr("Measure Volume"));
		toolBar->addWidget(measureVolButton);

		menu->addMenu(measureVolMenu);
	}
	else
	{
		groupButton->menu()->addAction(_action);
	}
}

void  MeasureTerrainVolume::onLeftButton()
{
  osg::ref_ptr<osg::Vec3Array>  refPlane = SetRefPlane::_vertexBP;

	if (SetRefPlane::_vertexBP.valid())
	{
		_refPlane.set(refPlane->at(0), refPlane->at(1), refPlane->at(2));
		_refPlane.makeUnitLength();
		_planeDone = true;
	}
	else
	{
    QMessageBox  msg(QMessageBox::Warning, tr("Error"), tr("Set basic plane first!"), QMessageBox::Ok);
		msg.setWindowModality(Qt::WindowModal);
		msg.exec();

		return;
	}

	if (_planeDone)
	{
		if (!_isDrawing)
    {
      _contour = new osg::Vec3Array;
    }

		_contour->push_back(_currentWorldPos);
		DrawSurfacePolygon::onLeftButton();
	}
}

void  MeasureTerrainVolume::onRightButton()
{
	if (_planeDone && _isDrawing)
	{
		_planeDone = false;
		DrawSurfacePolygon::onRightButton();
	}
}

void  MeasureTerrainVolume::onDoubleClick()
{
	if (_planeDone && _isDrawing)
	{
		_currentDrawNode->addDrawable(createPointGeode(_endPoint, _intersections.begin()->getLocalIntersectNormal()));

		drawOverlay();

		if (calculateVolume() == false)
    {
      onRightButton();
    }
    else
    {
      recordCurrent();
    }

    endDrawing();
		_planeDone = false;
	}
}

void  MeasureTerrainVolume::onMouseMove()
{
	if (_planeDone)
	{
		DrawSurfacePolygon::onMouseMove();
	}
}

float  MeasureTerrainVolume::volInBoundary(osg::Node *node, osg::Vec3Array *boundary)
{
  VolumeVisitor  vv(boundary, _refPlane, getOrAddPluginSettings("Measure highest level", false).toBool());

	node->accept(vv);

	return abs(vv.getVolume());
}

bool  MeasureTerrainVolume::calculateVolume()
{
  osg::ref_ptr<osg::Geometry>      polyg = tesselatedPolygon(_contour);
  osg::ref_ptr<osg::Vec3Array>     vertices;
  osg::ref_ptr<osg::PrimitiveSet>  prim;
  float                            vol = .0f;

	for (unsigned int numPrim = 0; numPrim < polyg->getNumPrimitiveSets(); numPrim++)
	{
    prim     = polyg->getPrimitiveSet(numPrim);
    vertices = (osg::Vec3Array *)polyg->getVertexArray();

    switch (prim->getMode())
		{
    case (osg::PrimitiveSet::TRIANGLES):

			for (unsigned int i = 0; i < prim->getNumIndices(); i += 3)
			{
        osg::ref_ptr<osg::Vec3Array>  ctrlPoints = new osg::Vec3Array;
				ctrlPoints->push_back(vertices->at(prim->index(i)));
        ctrlPoints->push_back(vertices->at(prim->index(i + 1)));
        ctrlPoints->push_back(vertices->at(prim->index(i + 2)));

				vol += volInBoundary(_overlayNode, ctrlPoints);
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

				vol += volInBoundary(_overlayNode, ctrlPoints);
			}

			break;
    case (osg::PrimitiveSet::TRIANGLE_FAN):

			for (unsigned int i = 1; i < prim->getNumIndices() - 1; i++)
			{
        osg::ref_ptr<osg::Vec3Array>  ctrlPoints = new osg::Vec3Array;
				ctrlPoints->push_back(vertices->at(prim->index(0)));
				ctrlPoints->push_back(vertices->at(prim->index(i)));
        ctrlPoints->push_back(vertices->at(prim->index(i + 1)));

				vol += volInBoundary(_overlayNode, ctrlPoints);
			}

			break;
		default:

			return false;
		}
	}

	_currentDrawNode->setUserValue("volume", vol);

  auto  str = tr("%1m3").arg(vol, 0, 'f', 1).toStdString();
  showTxtAtCenter(str);

	return true;
}
