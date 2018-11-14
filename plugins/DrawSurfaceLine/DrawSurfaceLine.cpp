#include "DrawSurfaceLine.h"

#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QToolButton>

#include <osg/LineWidth>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgUtil/IntersectionVisitor>

DrawSurfaceLine::DrawSurfaceLine()
{
	_pluginName = tr("Surface Line");
	_pluginCategory = "Draw";

	_slicer.setDatabaseCacheReadCallback(0);

    _zOffset = getOrAddPluginSettings("zOffset", 0.5).toDouble();

    QMap<QString, QVariant> customStyle;
    customStyle["Line color"] = QColor(0, 255, 0);
    customStyle["Point color"] = QColor(0, 0, 255);

    getOrAddPluginSettings("Draw style", customStyle);
}

void DrawSurfaceLine::onLeftButton()
{

	if (!_isDrawing)
	{
		beginDrawing();
		_startPoint = _anchoredWorldPos;
		_lastPoint = _startPoint;

		_currentDrawNode = new osg::Geode();
        _zOffsetNode = new osg::PositionAttitudeTransform;
        _zOffsetNode->setPosition({ 0, 0, _zOffset });
        _zOffsetNode->addChild(_currentDrawNode);
		_currentAnchor->addChild(_zOffsetNode);
	}
	else
	{
		_lastPoint = _anchoredWorldPos;
	}

    // Generate a new intersected line
	_lineGeom = newLine();
	_currentDrawNode->addDrawable(_lineGeom);
	_slicer.setStartPoint(_lastPoint + _anchoredOffset);

	_currentDrawNode->addDrawable(createPointGeode(_lastPoint, _intersections.begin()->getLocalIntersectNormal()));

}

void DrawSurfaceLine::onRightButton()
{
	if (_isDrawing)
	{
        endDrawing();
		_currentAnchor->removeChild(_zOffsetNode);
        _zOffsetNode = NULL;
        _currentDrawNode = NULL;
        _lineGeom = NULL;
	}
}

void DrawSurfaceLine::onDoubleClick()
{
	if (_isDrawing)
	{
        endDrawing();
		_currentDrawNode->addDrawable(createPointGeode(_endPoint, _intersections.begin()->getLocalIntersectNormal()));
        _zOffsetNode->setName(_pluginName.toStdString());

        osg::Vec3 testPoint;       
        recordNode(_zOffsetNode);
	}
}

void DrawSurfaceLine::onMouseMove()
{
	if (_isDrawing)
	{
		_endPoint = _anchoredWorldPos;
		if (_lastPoint != _endPoint)
		{
			_slicer.setEndPoint(_endPoint + _anchoredOffset);
			updateIntersectedLine();
		}
	}
}

osg::ref_ptr<osg::Geometry> DrawSurfaceLine::newLine()
{
	osg::ref_ptr<osg::Geometry> lineGeom = new osg::Geometry;
	lineGeom->setName("line");

	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(_style.lineColor);
	lineGeom->setColorArray(color.get(), osg::Array::BIND_OVERALL);

	lineGeom->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);

	return lineGeom;
}

osg::ref_ptr<osg::Vec3Array> DrawSurfaceLine::lineSmoothing(osg::ref_ptr<osg::Vec3Array> points)
{
	if (points->size() == 0)
		return nullptr;

	if (*(points->begin() + 1) == *points->rbegin() || *points->begin() == *points->rbegin())
		return points;

	osg::ref_ptr<osg::Vec3Array> arrPoints = new osg::Vec3Array;
	osg::Vec3Array::iterator iter = points->begin() + 1;
	arrPoints->push_back(*(iter - 1));
	osg::Vec3d posPre = *(iter - 1);
	osg::Vec3d posMid;
	osg::Vec3d posNext;
	while (1)
	{
		posMid = *iter;

		if ((iter + 1) == points->end())
			break;

		posNext = *(iter + 1);
		iter++;

		float avgZ = (posPre.z() + posNext.z()) / 2.0;
		if (posMid.z() < avgZ - 10 || 
            (abs(posPre.z() - posMid.z()) >= abs(posPre.z() - 5) && abs(posNext.z() - posMid.z()) >= abs(posNext.z() - 5)))
		{
			posPre = *(iter - 2);
		}
		else
		{
			posPre = *(iter - 1);
			arrPoints->push_back(posMid);
		}
	}
	return arrPoints;
}

inline void sortArray(osg::ref_ptr<osg::Vec3Array> array)
{
    if (array->begin()->x() <= array->rbegin()->x())
	{
        std::sort(array->begin(), array->end(), 
            [](const osg::Vec3d& p1, const osg::Vec3d& p2) { return p1.x() < p2.x(); }
        );
	}
	else
	{
        std::sort(array->begin(), array->end(),
            [](const osg::Vec3d& p1, const osg::Vec3d& p2) { return p1.x() > p2.x(); }
        );
	}
}

DrawSurfaceLine::~DrawSurfaceLine()
{
}

void DrawSurfaceLine::setupUi(QToolBar * toolBar, QMenu * menu)
{
	QIcon icon12;
	icon12.addFile(QStringLiteral("resources/icons/drawline.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("drawLinSrfAction"));
	_action->setCheckable(true);
	_action->setIcon(icon12);
	_action->setText(tr("Surface Line"));
	_action->setToolTip(tr("Draw Surface Line"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	QToolButton *groupButton = toolBar->findChild<QToolButton*>("DrawLineGroupButton", Qt::FindDirectChildrenOnly);
	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("DrawLineGroupButton");
		QIcon icon1;
		icon1.addFile(QString::fromUtf8("resources/icons/drawline.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(icon1);
		groupButton->setPopupMode(QToolButton::InstantPopup);
		groupButton->setCheckable(true);
		//groupButton->setDefaultAction(_action);
		groupButton->setText(tr("Line"));
		QMenu *drawLineMenu = new QMenu(groupButton);
		drawLineMenu->setTitle(tr("Line"));
		drawLineMenu->setIcon(icon1);
		drawLineMenu->addAction(_action);
		groupButton->setMenu(drawLineMenu);
		groupButton->setToolTip(tr("Draw Lines"));
		toolBar->addWidget(groupButton);

		menu->addMenu(drawLineMenu);
	}
	else
	{
		groupButton->menu()->addAction(_action);
	}
	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void DrawSurfaceLine::updateIntersectedLine()
{
    // Make intersection with all contents of the data root
    // The result will be in world coordinate
	_slicer.computeIntersections(_dataRoot);

	const osgSim::ElevationSlice::Vec3dList& pointList = _slicer.getIntersections();

	if (pointList.size() > 1)
	{
        // Convert array to anchored coord, sort and smoothing it
        osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array(pointList.begin(), pointList.end());
        sortArray(vertex);
        vertex = anchorArray(vertex);
        vertex = lineSmoothing(vertex);

		if (!vertex)
			return;

		_lineGeom->setVertexArray(vertex);

		_lineGeom->removePrimitiveSet(0, _lineGeom->getNumPrimitiveSets());
		_lineGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, vertex->size()));
		_lineGeom->dirtyDisplayList();
	}
}