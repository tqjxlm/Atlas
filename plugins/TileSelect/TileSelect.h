#pragma once
#include "TileSelect_global.h"
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace osg {
	class Geode;
}

class TileSelectDialog;

class TILESELECT_EXPORT TileSelect : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "TileSelect.json")
	Q_INTERFACES(PluginInterface)

public:
	TileSelect();
	~TileSelect();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
    virtual void onLeftButton() override;
    virtual void loadContextMenu(QMenu* contextMenu, QTreeWidgetItem* selectedItem) override;

signals:
	void tileSelected(const QString&);
	void tileUnSelected(const QString&);
	void closeTileSelectDialog();

public slots:
	void selectAllTilesSlots();
	void unselectAllTileSlots();

protected:
	virtual void toggle(bool checked) override;
    void selectTile(osg::Node* tileNode, osg::BoundingBox boundbox, osg::Vec3 worldCenter);
	void initTileSelectDialog(int itemTotalCount);
	void toggleTileSelectDialog(osg::Node* selectedNode, bool checked);

    // Get bound box from cache
    osg::BoundingBox getBound(osg::Node* node);

protected:
    osg::Node* _selectedNode = NULL;

	QList<QString> _nodeList;
    osg::Vec4 _selectedColor;

	TileSelectDialog* _tileSelectDlg = NULL;
	QAction* _action;

    static QMap<osg::Node*, osg::BoundingBox> _boundBuffers;
};
