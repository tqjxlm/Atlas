#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class Template : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "Template.json")
	Q_INTERFACES(PluginInterface)

public:
	Template();
	~Template();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
	virtual void trigger();

private:
	QAction* _action;
};

#endif // TEMPLATE_H
