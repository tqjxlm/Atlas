#include "VisibilityTest.h"

#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QToolBar>
#include <QToolButton>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/PositionAttitudeTransform>
#include <osgSim/OverlayNode>

VisibilityTest::VisibilityTest()
{
	_pluginName = tr("Visibility Test");
	_pluginCategory = "Analysis";

	_ptCount = 0;
	endDrawing();
	
	_visionVertex = new osg::Vec3Array;
	_noVisionVertex = new osg::Vec3Array;
}

VisibilityTest::~VisibilityTest(void)
{
}

void VisibilityTest::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("visibilityTestAction"));
	_action->setCheckable(true);
	QIcon icon;
	icon.addFile(QStringLiteral("resources/icons/vision.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon);
	_action->setText(tr("Line Visibility"));
	_action->setToolTip(tr("Visibility Test along Lines"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	QToolButton *groupButton = toolBar->findChild<QToolButton*>("VisibilityAnalysisGroupButton", Qt::FindDirectChildrenOnly);
	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("VisibilityAnalysisGroupButton");
		QIcon icon1;
		icon1.addFile(QString::fromUtf8("resources/icons/vision.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(icon1);
		groupButton->setPopupMode(QToolButton::InstantPopup);
		groupButton->setCheckable(true);
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
	//groupButton->setDefaultAction(_action);
	groupButton->setText(tr("Visibility")); 
	groupButton->setToolTip(tr("Visibility Analysis"));
	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void VisibilityTest::onLeftButton()
{
	if (!_isDrawing)
	{
		beginDrawing();

		initVisionGeode();

		_visionVertex->clear();
		_noVisionVertex->clear();

		_visionVertex->push_back(_anchoredWorldPos);

		_currentDrawNode = new osg::Geode;
		_currentDrawNode->addDrawable(_visionGeometry);
		_currentDrawNode->addDrawable(_noVisionGeometry);
		_currentAnchor->addChild(_currentDrawNode);
	}
	_ptCount++;

	if (_ptCount%2 == 0)
	{
		_currentDrawNode->addDrawable(createPointGeode(_first_intersection));
	}

	if (_ptCount%2 == 1&&_ptCount != 1)
	{
		_visionVertex->pop_back();
		_visionVertex->push_back(_anchoredWorldPos);
	}

	_currentDrawNode->addDrawable(createPointGeode(_anchoredWorldPos, _intersections.begin()->getLocalIntersectNormal()));

	_visionVertex->push_back(_anchoredWorldPos);

	_visionGeometry->setVertexArray(_visionVertex);
	_start_point = _currentWorldPos;
	_start_point.z() += 1.8;

	_visionGeometry->removePrimitiveSet(0);
	_visionGeometry->addPrimitiveSet(new osg::DrawArrays(osg::DrawArrays::LINES, 0, _visionVertex->size()));
	_visionGeometry->dirtyDisplayList();
	
	if (_ptCount%2 == 1)
	{
		_noVisionVertex->push_back(_anchoredWorldPos);
		_noVisionVertex->push_back(_anchoredWorldPos);
	}

	_noVisionGeometry->removePrimitiveSet(0);
	_noVisionGeometry->addPrimitiveSet(new osg::DrawArrays(osg::DrawArrays::LINES, 0, _noVisionVertex->size()));
	_noVisionGeometry->dirtyDisplayList();
}

void VisibilityTest::onRightButton()
{
	if (_isDrawing)
	{
		_currentAnchor->removeChild(_currentDrawNode);
		endDrawing();
		_ptCount = 0;
	}
}

void VisibilityTest::onDoubleClick()
{
	if (_isDrawing)
	{
		this->recordCurrent();
		endDrawing();
		_ptCount = 0;

	}
}

void VisibilityTest::onMouseMove()
{
	if (_isDrawing)
	{
		_end_point = _currentWorldPos;
		osgUtil::LineSegmentIntersector::Intersections intersections;  
		osg::ref_ptr<osgUtil::LineSegmentIntersector> ls = new osgUtil::LineSegmentIntersector(_start_point, _end_point);  
		osg::ref_ptr<osgUtil::IntersectionVisitor> iv = new osgUtil::IntersectionVisitor(ls);  
		_overlayNode->accept(*iv);

		if(ls->containsIntersections())  
		{  
			intersections = ls->getIntersections();  
			for(osgUtil::LineSegmentIntersector::Intersections::iterator iter = intersections.begin();iter != intersections.end();iter++)  
			{  
				_first_intersection = iter->getWorldIntersectPoint() - _anchoredOffset;
				break;
			}  
		}

		_visionVertex->pop_back();
		_visionVertex->push_back(_first_intersection);
		_visionGeometry->setVertexArray(_visionVertex);
		_visionGeometry->dirtyDisplayList();
		
		if (_ptCount%2 == 1)
		{
			_noVisionVertex->pop_back();
			_noVisionVertex->pop_back();
			_noVisionVertex->push_back(_first_intersection);
			_noVisionVertex->push_back(_end_point-_anchoredOffset);
			_noVisionGeometry->setVertexArray(_noVisionVertex);
			_noVisionGeometry->dirtyDisplayList();
		}
	}
}

void VisibilityTest::initVisionGeode()
{
	_visionGeometry = new osg::Geometry;

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(osg::Vec4(0.0, 1.0, 0.0, 1.0));
	_visionGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);

	_visionGeometry->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
	
	_noVisionGeometry = new osg::Geometry;

	osg::ref_ptr<osg::Vec4Array> noVi_colors = new osg::Vec4Array;
	noVi_colors->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
	_noVisionGeometry->setColorArray(noVi_colors, osg::Array::BIND_OVERALL);

	_noVisionGeometry->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
}