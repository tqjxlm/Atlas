#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
class QTreeWidgetItem;
QT_END_NAMESPACE

class SlopAnalysis : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "SlopAnalysis.json")
	Q_INTERFACES(PluginInterface)

public:
	SlopAnalysis();
	~SlopAnalysis();
    virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

public slots:
	virtual void toggle(bool checked) override;

protected:
    osg::Node* _selectedNode;
    osg::ref_ptr<osg::Program> _slopeProgram;

private:
	QAction* _action;
};
