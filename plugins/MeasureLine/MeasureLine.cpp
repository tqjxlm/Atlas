#include "MeasureLine.h"

#include <QAction>
#include <QToolBar>
#include <QMenu>

#include <osgText/Text>

MeasureLine::MeasureLine()
{
  _pluginName     = tr("Distance");
  _pluginCategory = "Measure";
  _totoalDistance = 0;

  QMap<QString, QVariant>  customStyle;
  customStyle["Text color"]  = QColor(253, 74, 63);
  customStyle["Line color"]  = QColor(155, 255, 92);
  customStyle["Point color"] = QColor(255, 255, 0);

  getOrAddPluginSettings("Draw style", customStyle);
}

MeasureLine::~MeasureLine(void)
{
}

void  MeasureLine::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("measureLineAction"));
	_action->setCheckable(true);
  QIcon  icon;
	icon.addFile(QStringLiteral("resources/icons/ruler.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon);
	_action->setText(tr("Distance"));
	_action->setToolTip(tr("Measure Distance along Line"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	registerMutexAction(_action);

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void  MeasureLine::onLeftButton()
{
	DrawLine::onLeftButton();

	if (_isDrawing)
	{
    int  numVert = _verticsLine->size();

		if (numVert > 2)
		{
      double     currentDis = (_verticsLine->at(numVert - 3) - _verticsLine->at(numVert - 2)).length();
      osg::Vec3  disPos     = (_verticsLine->at(numVert - 3) + _verticsLine->at(numVert - 2)) / 2;

			_totoalDistance += currentDis;

      auto  str = tr("%1m").arg(currentDis, 0, 'f', 1).toStdString();
      _currentDrawNode->addDrawable(createTextGeode(str, disPos));
		}
	}
}

void  MeasureLine::onDoubleClick()
{
	if (_isDrawing)
	{
    _style.textColor = { 1.0f, 1.0f, 0.0f, 1.0f };
    auto  str = tr("%1m").arg(_totoalDistance, 0, 'f', 1).toStdString();
    _currentDrawNode->addDrawable(createTextGeode(str, _anchoredWorldPos));
		_totoalDistance = 0;
    _currentDrawNode->removeChild(_tmpLabel);
    _tmpLabel = NULL;
  }

	DrawLine::onDoubleClick();
}

void  MeasureLine::onMouseMove()
{
  DrawLine::onMouseMove();

  if (_isDrawing)
  {
    int  numVert = _verticsLine->size();

    if (numVert > 1)
    {
      double  currentDis = (_verticsLine->at(numVert - 2) - _verticsLine->at(numVert - 1)).length();

      if (!_tmpLabel.valid())
      {
        auto  str = tr("%1m").arg(currentDis, 0, 'f', 1).toStdString();
        _tmpLabel = createTextGeode(str, _anchoredWorldPos);
        _currentDrawNode->addDrawable(_tmpLabel);
      }

      _tmpLabel->setText(tr("%1m").arg(currentDis, 0, 'f', 1).toStdString());
      _tmpLabel->setPosition(_anchoredWorldPos + osg::Vec3(0, 0, _style.textFloating));
    }
  }
}
