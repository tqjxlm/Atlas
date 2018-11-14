#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class MeshMode : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "MeshMode.json")
	Q_INTERFACES(PluginInterface)

public:
	MeshMode();
	~MeshMode();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
	virtual void trigger();

protected:
	QAction* _action;
	int _mode;
};
