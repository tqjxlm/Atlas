#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

#include <QMutex>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class AddObliqueModel : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "AddObliqueModel.json")
	Q_INTERFACES(PluginInterface)

public:
	AddObliqueModel();
	~AddObliqueModel();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected slots:
	void addObliqueModel();

protected:
	void loadObliqueModel(const QString& pathXML);

private:
	QMutex _loadingLock;
};
