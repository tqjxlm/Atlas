#include "TileSelect.h"

#include <QMessageBox>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QTreeWidgetItem>

#include <osg/ComputeBoundsVisitor>
#include <osg/PositionAttitudeTransform>
#include <osg/ShapeDrawable>
#include <osg/BoundingBox>

#include <DataManager/DataManager.h>
#include <DataManager/DataRecord.h>
#include <ViewerWidget/ViewerWidget.h>

#include <DataManager/FindNode.hpp>
#include "TileSelectDialog.h"

static osg::Vec4 colorToVec(const QColor &color)
{
    return osg::Vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

QMap<osg::Node*, osg::BoundingBox> TileSelect::_boundBuffers;

TileSelect::TileSelect()
{
    _pluginName = tr("Select Tile");
    _pluginCategory = "Analysis";

    QMap<QString, QVariant> customStyle;
    customStyle["Fill color"] = QColor(188, 255, 94, 128);

    customStyle = getOrAddPluginSettings("Draw style", customStyle).toMap();

    _selectedColor = colorToVec(customStyle["Fill color"].value<QColor>());
}

TileSelect::~TileSelect()
{
    if (_tileSelectDlg)
    {
        _tileSelectDlg->close();
    }
}

void TileSelect::setupUi(QToolBar * toolBar, QMenu * menu)
{
    _action = new QAction(_mainWindow);
    _action->setObjectName(QStringLiteral("tileSelectAction"));
    _action->setCheckable(true);
    QIcon icon;
    icon.addFile(QStringLiteral("resources/icons/tile.png"), QSize(), QIcon::Normal, QIcon::Off);
    _action->setIcon(icon);
    _action->setText(tr("Tiles"));
    _action->setToolTip(tr("Shows Tiles of Oblique Models"));

    connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
    registerMutexAction(_action);
}

void TileSelect::onLeftButton()
{
    if (!_activated)
        return;

    for (auto intersection : _intersections)
    {
        for (auto it = intersection.nodePath.begin(); it != intersection.nodePath.end(); it++)
        {
            if (*it == _selectedNode)
            {
                // Draw bound
                osg::Node* node = *std::next(it);
                osg::Matrix trans = osg::computeLocalToWorld(_selectedNode->getParentalNodePaths().front());
                auto boundbox = getBound(node);
                _style.fillColor = _selectedColor;
                selectTile(node, boundbox, boundbox.center() * trans);
                return;
            }
        }
    }
}

void TileSelect::loadContextMenu(QMenu * contextMenu, QTreeWidgetItem * selectedItem)
{
    if (selectedItem->parent()->text(0) == tr("Oblique Imagery Model"))
    {
        auto dataRecord = dynamic_cast<DataRecord*>(selectedItem);
        if (dataRecord && !dataRecord->isLayer() && dataRecord->node())
        {
            contextMenu->addAction(_action);
            _selectedNode = dataRecord->node();
        }
    }
}

void TileSelect::toggle(bool checked)
{
    if (checked)
    {
        osg::Matrix trans = osg::computeLocalToWorld(_selectedNode->getParentalNodePaths().front());
        _currentAnchor = getNearestAnchorPoint(_selectedNode->getBound().center() * trans);
        _anchoredOffset = _currentAnchor->getPosition();

        _currentDrawNode = new osg::Geode;
        _currentAnchor->addChild(_currentDrawNode);
        osg::ref_ptr<osg::StateSet> stateset = _currentDrawNode->getOrCreateStateSet();
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        //toggleTileSelectDialog(_selectedNode, true);
        selectAllTilesSlots();
    }
    else
    {
        //toggleTileSelectDialog(NULL, false);
        unselectAllTileSlots();

        _currentAnchor->removeChild(_currentDrawNode);
        _currentDrawNode = NULL;
    }

    _activated = checked;
}

void TileSelect::selectTile(osg::Node* tileNode, osg::BoundingBox boundbox, osg::Vec3 worldCenter)
{
    QString tileName = QString::fromStdString(tileNode->getName());

    int drawedNum = _currentDrawNode->getNumDrawables();
    for (int i = 0; i < drawedNum; i++)
    {
        osg::Drawable *drawobj = _currentDrawNode->getDrawable(i);
        if (drawobj->getName() == tileNode->getName())
        {
            _currentDrawNode->removeDrawable(drawobj);
            _nodeList.removeAt(_nodeList.indexOf(tileName));

            emit tileUnSelected(tileName);
            return;
        }
    }

    worldCenter -= _anchoredOffset;
    float xwidth = boundbox.xMax() - boundbox.xMin();
    float ywidth = boundbox.yMax() - boundbox.yMin();
    float zwidth = boundbox.zMax() - boundbox.zMin();

    osg::ref_ptr<osg::Box>  box = new osg::Box(worldCenter, xwidth, ywidth, zwidth);

    auto center = boundbox.center();
    osg::Vec3 vecBoundingbox = osg::Vec3(
        boundbox.xMin(), boundbox.yMin(), 0) - osg::Vec3(center.x(), center.y(), 0);
    osg::Vec3 vecBox = osg::Vec3(center.x() - 10, center.y() - 10, center.z()) - center;
    osg::Quat quat;
    quat.makeRotate(vecBox, vecBoundingbox);
    box->setRotation(quat);

    osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(box);
    drawable->setColor(_style.fillColor);
    drawable->setName(tileNode->getName());

    _currentDrawNode->addDrawable(drawable);

    emit tileSelected(tileName);

    _nodeList.append(tileName);
}

osg::BoundingBox TileSelect::getBound(osg::Node* node)
{
    osg::BoundingBox boundbox;
    if (!_boundBuffers.contains(node))
    {
        osg::ComputeBoundsVisitor boundsVisitor;

        osg::PagedLOD* lod = dynamic_cast<osg::PagedLOD*>(node);
        osg::ref_ptr<osg::Transform> transNode = new osg::Transform;
        transNode->addChild(lod);
        boundsVisitor.reset();
        boundsVisitor.apply(*transNode);

        boundbox = boundsVisitor.getBoundingBox();
        _boundBuffers[node] = boundbox;
    }
    else
    {
        boundbox = _boundBuffers[node];
    }
    return boundbox;
}

void TileSelect::selectAllTilesSlots()
{
    unselectAllTileSlots();

    osg::Group* activeModeNode = _selectedNode->asGroup();
    osg::Matrix trans = osg::computeLocalToWorld(activeModeNode->getParentalNodePaths().front());

    for (unsigned i = 0; i < activeModeNode->getNumChildren(); i++)
    {
        auto node = activeModeNode->getChild(i);
        auto boundbox = getBound(node);

        _style.fillColor = _selectedColor;
        selectTile(node, boundbox, boundbox.center() * trans);
    }
}

void TileSelect::unselectAllTileSlots()
{
    _currentDrawNode->removeChildren(0, _currentDrawNode->getNumChildren());
    _nodeList.clear();
}

void TileSelect::initTileSelectDialog(int itemTotalCount)
{
    _tileSelectDlg = new TileSelectDialog(itemTotalCount, _mainWindow);
    connect(this, SIGNAL(tileSelected(const QString&)), _tileSelectDlg, SLOT(selectTileSlot(const QString&)), Qt::UniqueConnection);
    connect(this, SIGNAL(tileUnSelected(const QString&)), _tileSelectDlg, SLOT(unselectTileSlot(const QString&)), Qt::UniqueConnection);
    connect(_tileSelectDlg, SIGNAL(closed()), _action, SLOT(toggle()));
    connect(_tileSelectDlg, SIGNAL(selectAllTile()), this, SLOT(selectAllTilesSlots()), Qt::UniqueConnection);
    connect(_tileSelectDlg, SIGNAL(unselectAllTile()), this, SLOT(unselectAllTileSlots()), Qt::UniqueConnection);
}

void TileSelect::toggleTileSelectDialog(osg::Node* selectedNode, bool checked)
{
    if (checked)
    {
        if (!_tileSelectDlg)
        {
            initTileSelectDialog(selectedNode->asGroup()->getNumChildren());
        }
        _tileSelectDlg->show();
    }
    else
    {
        if (_tileSelectDlg)
        {
            _tileSelectDlg = NULL;
        }
    }
}
