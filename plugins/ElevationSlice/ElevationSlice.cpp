#include "ElevationSlice.h"

#include <QAction>
#include <QMenu>
#include <QToolBar>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

#include "ElevationSliceShowDlg.h"

ElevationSlice::ElevationSlice()
{
	_pluginName = tr("Elevation Slice Analysis");
	_pluginCategory = "Analysis";

	_dialogSlice = NULL;
}

ElevationSlice::~ElevationSlice()
{
}

void ElevationSlice::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("elevationSliceAction"));
	_action->setCheckable(true);
	QIcon icon28;
	icon28.addFile(QStringLiteral("resources/icons/curve.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon28);
	_action->setText(tr("Slice"));
	_action->setToolTip(tr("Elevation Slice Analysis"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void ElevationSlice::onLeftButton()
{	
	if (!_dialogSlice)
	{
		_dialogSlice = new ElevationSliceShowDlg();
		connect(_dialogSlice, SIGNAL(cancelAnalysis()), this, SLOT(clearDraw()));
		_dialogSlice->show();
	}

	if (!_isDrawing)
	{
		beginDrawing();

		if (_currentDrawNode&&_gemList.size()>0)
		{
			for (int i = 0; i<_gemList.size(); i++)
			{
				osg::Geometry* p = _gemList.at(i);
				_currentDrawNode->removeDrawable(p);
			}
			_gemList.clear();
		}

		_startPoint = _anchoredWorldPos;
		_lastPoint = _startPoint;

		_currentDrawNode = new osg::Geode();
		_currentDrawNode->setUserValue("distance", 0.0f);
		_currentAnchor->addChild(_currentDrawNode);

		_slicerPointList = new osg::Vec3Array;
	}
	else
	{
		osg::Vec3Array* lastPoints = (osg::Vec3Array*)(_lineGeom->getVertexArray());
		if (!lastPoints->empty())
		_slicerPointList->insert(_slicerPointList->end(), lastPoints->begin(), lastPoints->end());

		_gemList.append(_lineGeom);

		// 设置新起点
		_lastPoint = _anchoredWorldPos;

	}

	// Begin drawing
	_lineGeom = newLine();
	_currentDrawNode->addDrawable(_lineGeom.get());

	_slicer.setStartPoint(_lastPoint);

	// Start wiith a point
	osg::ref_ptr<osg::Geometry> pointgem = createPointGeode(_lastPoint, _intersections.begin()->getLocalIntersectNormal());
	_currentDrawNode->addDrawable(pointgem);
	_gemList.append(pointgem);
	//DrawSurfaceLine::onLeftButton();

}

void ElevationSlice::onRightButton()
{
	DrawSurfaceLine::onRightButton();
	if (_currentDrawNode)
	{
		int childcount = _currentDrawNode->getNumDrawables();
		for (int i=0;i<childcount;i++)
		{
			_currentDrawNode->removeDrawables(i);
		}
	}
}

void ElevationSlice::onMouseMove()
{
	DrawSurfaceLine::onMouseMove();

}
void ElevationSlice::onDoubleClick()
{
	if (_isDrawing)
	{
		endDrawing();
		// Start wiith a point
		_currentDrawNode->addDrawable(createPointGeode(_endPoint, _intersections.begin()->getLocalIntersectNormal()));
		 
		sliceAnalysis();
	}
}

void ElevationSlice::clearDraw()
{
	if (_currentDrawNode&&_gemList.size()>0)
	{
		for (int i=0;i<_gemList.size();i++)
		{
			osg::Geometry* p=_gemList.at(i);
			_currentDrawNode->removeDrawable(p);
		}
		_gemList.clear();
	}
}
void ElevationSlice::sliceAnalysis()
{
	osg::ref_ptr<osg::Vec3Array> arr = _slicerPointList;

	if (arr->size() < 3)
		return;

	osg::ref_ptr<osg::FloatArray> arrPointDistance = new osg::FloatArray;//存储每个点与起点的距离

	float distance = 0;

	for (int i=0;i<arr->size();i++)
	{
		arr->at(i)+=_anchoredOffset;

		if(i>0)
			distance += (arr->at(i) - arr->at(i - 1)).length();

		arrPointDistance->push_back(distance);
	}


	if (!_dialogSlice)
	{
		_dialogSlice = new ElevationSliceShowDlg();
	   connect(_dialogSlice,SIGNAL(cancelAnalysis()),this,SLOT(clearDraw()));
	   _dialogSlice->show();
	}

	_dialogSlice->setShowPoint(arrPointDistance,arr);

	_dialogSlice->show();

}