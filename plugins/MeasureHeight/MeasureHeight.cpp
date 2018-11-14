#include "MeasureHeight.h"

#include <osgText/Text>
#include <osg/Geometry>
#include <osg/LineWidth>
#include <osg/PositionAttitudeTransform>

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QToolBar>

MeasureHeight::MeasureHeight():
	_height(0)
{
	_pluginName = tr("Height");
	_pluginCategory = "Measure";

    QMap<QString, QVariant> customStyle;
    customStyle["Text color"] = QColor(255, 253, 61);
    customStyle["Line color"] = QColor(255, 221, 68);
    customStyle["Line width"] = 2.0f;

    getOrAddPluginSettings("Draw style", customStyle);
}

MeasureHeight::~MeasureHeight(void)
{
}

void MeasureHeight::setupUi(QToolBar * toolBar, QMenu* menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("measHeiAction"));
	_action->setCheckable(true);
	QIcon icon11;
	icon11.addFile(QStringLiteral("resources/icons/height.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon11);
	_action->setText(tr("Height"));
	_action->setToolTip(tr("Measure Height"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void MeasureHeight::onLeftButton()
{
	if (!_isDrawing )
	{
		beginDrawing();
		_startPoint = _anchoredWorldPos;

		_currentDrawNode = new osg::Geode();
        _currentDrawNode->getOrCreateStateSet()->setAttribute(_style.lineWidth);
		_currentAnchor->addChild(_currentDrawNode);

		_hLineUp = new osg::Geometry;
		_hLineLow = new osg::Geometry;
		_vLine = new osg::Geometry;
		_currentDrawNode->addDrawable(_hLineUp);
		_currentDrawNode->addDrawable(_hLineLow);
		_currentDrawNode->addDrawable(_vLine);
	}
	else
	{
		endDrawing();
		_text = NULL;
		recordCurrent();
	}

	createPointGeode(_anchoredWorldPos, _intersections.begin()->getLocalIntersectNormal());
}

void MeasureHeight::onRightButton()
{
	if (_isDrawing)
	{
		endDrawing();
		_currentAnchor->removeChild(_currentDrawNode);
		_text = NULL;
	}
}

void MeasureHeight::onMouseMove()
{
	if (_isDrawing)
	{
		_height = _anchoredWorldPos.z() - _startPoint.z();
		if (_height > 0)
		{
			_highPoint = _anchoredWorldPos;
			_lowPoint = _startPoint;
		}
		else
		{
			_height *= -1;
			_highPoint = _startPoint;
			_lowPoint = _anchoredWorldPos;
		}
		
		_hDir = _highPoint - _lowPoint;
		_hDir.z() = 0;
		_hDir.normalize();

		updateLines();
		updateText();
	}
}

void MeasureHeight::updateLines()
{
    float crossWidth = 2.0f;

	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(_style.lineColor);

    osg::Vec3 hDirPerp = osg::Vec3(_hDir.y(), -_hDir.x(), 0);

	osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
	vertices->push_back(_highPoint);
	vertices->push_back(osg::Vec3(_lowPoint.x(), _lowPoint.y(), _highPoint.z()) - _hDir * crossWidth);
    vertices->push_back(osg::Vec3(_lowPoint.x(), _lowPoint.y(), _highPoint.z()) + hDirPerp * crossWidth);
    vertices->push_back(osg::Vec3(_lowPoint.x(), _lowPoint.y(), _highPoint.z()) - hDirPerp * crossWidth);
	_hLineUp->setVertexArray(vertices);
	_hLineUp->setColorArray(color, osg::Array::BIND_OVERALL);
	_hLineUp->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
	_hLineUp->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
	_hLineUp->dirtyDisplayList();

	vertices = new osg::Vec3Array;
	vertices->push_back(_lowPoint + _hDir * crossWidth);
	vertices->push_back(_lowPoint - _hDir * crossWidth);
    vertices->push_back(_lowPoint + hDirPerp * crossWidth);
    vertices->push_back(_lowPoint - hDirPerp * crossWidth);
	_hLineLow->setVertexArray(vertices);
	_hLineLow->setColorArray(color, osg::Array::BIND_OVERALL);
	_hLineLow->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
	_hLineLow->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
	_hLineLow->dirtyDisplayList();

	vertices = new osg::Vec3Array;
	vertices->push_back(_lowPoint);
	vertices->push_back(osg::Vec3(_lowPoint.x(), _lowPoint.y(), _highPoint.z()));
	_vLine->setVertexArray(vertices);
	_vLine->setColorArray(color, osg::Array::BIND_OVERALL);
	_vLine->getOrCreateStateSet()->setAttributeAndModes(_style.lineWidth, osg::StateAttribute::ON);
	_vLine->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, vertices->size()));
	_vLine->dirtyDisplayList();
}

void MeasureHeight::updateText() 
{  
	std::string txt = tr("%1m").arg(_height, 0, 'f', 1).toStdString();
	osg::Vec3 position = _lowPoint + osg::Vec3(0, 0, _height / 2);

	if (!_text.valid())
	{
		_text = createTextGeode(txt, position);
		_currentDrawNode->addDrawable(_text);
	}
	else
	{
		_text->setPosition( position );
		_text->setText(txt); 
		_text->dirtyDisplayList();
	}
}
