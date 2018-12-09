#pragma once
#include <QtPlugin>
#include <DrawSurfacePolygon/DrawSurfacePolygon.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

#include "SaveOrthoProjDialog.h"

class OrthoMap : public DrawSurfacePolygon
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "OrthoMap.json")
	Q_INTERFACES(PluginInterface)

public:
	OrthoMap();
	~OrthoMap();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
  virtual void loadContextMenu(QMenu * contextMenu, QTreeWidgetItem * selectedItem) override;
  virtual void onDoubleClick();
	virtual void onRightButton();
	virtual void setupWorkingDialog(osg::Node* scene);

public slots:
	virtual void toggle(bool checked) override;

protected:
    osg::Node* _selectedNode;
	SaveOrthoProjDialog::ProjectionMode _mode;
	SaveOrthoProjDialog* _saveDialog;
	QAction* _orthoModelAction;
	QAction* _orthoAreaAction;
	QAction* _orthoModelDSMAction;
	QAction* _orthoAreaDSMAction;
};
