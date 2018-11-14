#include "DrawLine.h"

#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QToolButton>
#include <QDebug>

#include <osg/LineWidth>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

DrawLine::DrawLine()
{
	_pluginName = tr("Straight Line");
	_pluginCategory = "Draw";
}

DrawLine::~DrawLine(void)
{
}

void DrawLine::setupUi(QToolBar * toolBar, QMenu * menu)
{
	QIcon icon;
	icon.addFile(QStringLiteral("resources/icons/drawline.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("drawLineAction"));
	_action->setCheckable(true);
	_action->setIcon(icon);
	_action->setText(tr("Line"));
	_action->setToolTip(tr("Draw Line"));

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
	//groupButton->setDefaultAction(_action);
	groupButton->setText(tr("Line"));
	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void DrawLine::onLeftButton()
{
	if (!_isDrawing)
	{
		beginDrawing();

		newLine();

		_verticsLine->push_back(_anchoredWorldPos);

		_currentDrawNode = new osg::Geode;
		_currentDrawNode->addDrawable(_lineGeometry);
		_currentAnchor->addChild(_currentDrawNode);
	}

	_currentDrawNode->addDrawable(createPointGeode(_anchoredWorldPos, _intersections.begin()->getLocalIntersectNormal()));

	_verticsLine->push_back(_anchoredWorldPos);
	_verticsLine->dirty();
	_lineGeometry->setVertexArray(_verticsLine);
	_lineGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, _verticsLine->size()));
}

void DrawLine::onRightButton()
{
	if (_isDrawing)
	{
        endDrawing();
        _currentAnchor->removeChild(_currentDrawNode);

        _currentDrawNode = NULL;
        _lineGeometry = NULL;
        _verticsLine = NULL;
	}
	else
	{
		_action->toggle();
	}
}

void DrawLine::onDoubleClick()
{
	if (_isDrawing)
	{
		recordNode(_currentDrawNode, _pluginName);
        endDrawing();
	}
}

void DrawLine::onMouseMove()
{
	if (_isDrawing)
	{
		_verticsLine->pop_back();
		_verticsLine->push_back(_anchoredWorldPos);
		_verticsLine->dirty();
		_lineGeometry->setVertexArray(_verticsLine);
		_lineGeometry->dirtyDisplayList();
	}
}

void DrawLine::newLine()
{
	_lineGeometry = new osg::Geometry;

	osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
	colors->push_back(_style.lineColor);
	_lineGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);
	
	_lineGeometry->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);

	_verticsLine = new osg::Vec3Array;
}
