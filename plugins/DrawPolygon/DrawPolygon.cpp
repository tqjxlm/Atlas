#include "DrawPolygon.h"

#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QToolButton>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

DrawPolygon::DrawPolygon()
{
	_pluginName = tr("Polygon");
	_pluginCategory = "Draw";
}

DrawPolygon::~DrawPolygon(void)
{
}

void DrawPolygon::setupUi(QToolBar * toolBar, QMenu * menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("drawPolygonAction"));
	_action->setCheckable(true);
	QIcon icon;
	icon.addFile(QStringLiteral("resources/icons/drawpolygon.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon);
	_action->setText(tr("Polygon"));
	_action->setToolTip(tr("Draw Polygon"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	QToolButton *groupButton = toolBar->findChild<QToolButton*>("DrawPolygonGroupButton", Qt::FindDirectChildrenOnly);
	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("DrawPolygonGroupButton");
		QIcon icon2;
		icon2.addFile(QString::fromUtf8("resources/icons/drawpolygon.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(icon2);
		groupButton->setPopupMode(QToolButton::InstantPopup);
		groupButton->setCheckable(true);
		QMenu* drawPlgMenu = new QMenu(groupButton);
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
	//groupButton->setDefaultAction(_action);
	groupButton->setText(tr("Polygon"));
	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void DrawPolygon::onLeftButton()
{
	if (_isDrawing==false)
	{
        beginDrawing();
        _currentDrawNode = new osg::Geode;
		_currentAnchor->addChild(_currentDrawNode);
		_polyVecArray = new osg::Vec3Array;
	}

	_currentDrawNode->addDrawable(createPointGeode(_anchoredWorldPos, _intersections.begin()->getLocalIntersectNormal()));
	_polyVecArray->push_back(_anchoredWorldPos);
}

void DrawPolygon::onRightButton()
{
	if (_isDrawing)
	{
        endDrawing();
        _currentAnchor->removeChild(_currentDrawNode);
		_polyVecArray->clear();
	}
}

void DrawPolygon::onMouseMove()
{
	if (_isDrawing && _polyVecArray != NULL)
	{
		if (_polyVecArray->size()<2)
		{
			_polyVecArray->push_back(_anchoredWorldPos);
			_currentDrawNode->addDrawable(createPolygon());
			return;
		}
		_polyVecArray->pop_back();
		_polyVecArray->push_back(_anchoredWorldPos);
		_currentDrawNode->removeDrawables(1);
		_currentPolygon = createPolygon();
		_currentDrawNode->addDrawable(_currentPolygon);
	}
}

void DrawPolygon::onDoubleClick()
{
	if (_isDrawing)
	{
        endDrawing();
		recordCurrent();
	}
}

osg::ref_ptr<osg::Geometry> DrawPolygon::createPolygon()
{
	osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
	geom->setVertexArray(_polyVecArray);
	geom->setName("polygon");

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(_style.fillColor);
	geom->setColorArray(colors, osg::Array::BIND_OVERALL);

	osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
	normals->push_back(osg::Vec3(0.f,0.f,1.f));

	geom->setNormalArray(normals, osg::Array::BIND_OVERALL);
	geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN,0,_polyVecArray->size()));
	return geom.get();
}
