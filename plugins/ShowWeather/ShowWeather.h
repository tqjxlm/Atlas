#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
class QDockWidget;
class QTabWidget;
class QSpinBox;
QT_END_NAMESPACE

namespace osgParticle {
	class PrecipitationEffect;
}

class ShowWeather : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "ShowWeather.json")
	Q_INTERFACES(PluginInterface)

public:
	ShowWeather();
	~ShowWeather();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

protected:
	virtual void toggle(bool checked) override;
	void initPanelTabPage();

protected slots:
	void showRainSimActionSlot(int rainSize);
	void showSnowSimActionSlot(int snowSize);

protected:
	int _tabIndex;
	QDockWidget* _controlPanel;
	QTabWidget* _tabWidget;

	osg::ref_ptr<osgParticle::PrecipitationEffect> _snowEffect;
	osg::ref_ptr<osgParticle::PrecipitationEffect> _rainEffect;

    int _maxSnow;
    int _maxRain;

	QAction* _action;
};
