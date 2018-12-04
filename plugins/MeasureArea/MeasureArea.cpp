#include "MeasureArea.h"

#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QToolButton>

#include <osgText/Text>

MeasureArea::MeasureArea()
{
  _pluginName     = tr("Area");
  _pluginCategory = "Measure";
}

MeasureArea::~MeasureArea(void)
{
}

void  MeasureArea::setupUi(QToolBar *toolBar, QMenu *menu)
{
  _action = new QAction(_mainWindow);
  _action->setObjectName(QStringLiteral("measureAreaAction"));
  _action->setCheckable(true);
  QIcon  icon20;
  icon20.addFile(QStringLiteral("resources/icons/area.png"), QSize(), QIcon::Normal, QIcon::Off);
  _action->setIcon(icon20);
  _action->setText(tr("Polygon Area"));
  _action->setToolTip(tr("Measure Polygon Area"));

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

  // groupButton->setDefaultAction(_action);
  groupButton->setText(tr("Area"));
  connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

inline float  computeTriangleArea(float a, float b, float c)
{
  float  p = (a + b + c) / 2;
  float  s = sqrt(p * (p - a) * (p - b) * (p - c));

  return s;
}

float  MeasureArea::calcuSpatialPolygonArea()
{
  osg::ref_ptr<osg::Vec3Array>     vertices;
  osg::ref_ptr<osg::PrimitiveSet>  prim;
  float                            area = 0;

  for (unsigned int numPrim = 0; numPrim < _currentPolygon->getNumPrimitiveSets(); numPrim++)
  {
    prim     = _currentPolygon->getPrimitiveSet(numPrim);
    vertices = (osg::Vec3Array *)_currentPolygon->getVertexArray();

    for (unsigned int i = 1; i < prim->getNumIndices() - 1; i++)
    {
      osg::Vec3  pos1 = vertices->at(prim->index(0));
      osg::Vec3  pos2 = vertices->at(prim->index(i));
      osg::Vec3  pos3 = vertices->at(prim->index(i + 1));

      float  length1 = (pos1 - pos2).length();
      float  length2 = (pos3 - pos2).length();
      float  length3 = (pos1 - pos3).length();

      area += computeTriangleArea(length1, length2, length3);
    }
  }

  return area;
}

void  MeasureArea::onDoubleClick()
{
  if (_isDrawing)
  {
    endDrawing();
    this->recordCurrent();
    osg::Vec3  posMidlle;

    for (int i = 0; i < _polyVecArray->size(); i++)
    {
      posMidlle += _polyVecArray->at(i);
    }

    posMidlle = posMidlle / _polyVecArray->size();
    auto  str = tr("%1m2").arg(calcuSpatialPolygonArea(), 0, 'f', 1).toStdString();
    _currentDrawNode->addDrawable(
      createTextGeode(str, posMidlle + _anchoredWorldPos));
  }
}
