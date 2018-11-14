#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class ScreenShot : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "ScreenShot.json")
	Q_INTERFACES(PluginInterface)

public:
	ScreenShot();
	~ScreenShot();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
	virtual void trigger();

private:
	QAction* _action;
};
