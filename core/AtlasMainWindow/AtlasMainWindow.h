#ifndef ATLASMAINWINDOW_H
#define ATLASMAINWINDOW_H

#include "atlasmainwindow_global.h"

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
class QProgressBar;
class QLabel;
QT_END_NAMESPACE

class NXDockWidget;
class NXDockWidgetTabBar;

namespace Ui {
	class AtlasMainWindowClass;
}

class ATLASMAINWINDOW_EXPORT AtlasMainWindow : public QMainWindow
{
public:
	AtlasMainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
	~AtlasMainWindow();

	// Add an auto-hide dock widget
	void addDockWidget(Qt::DockWidgetArea area, NXDockWidget* dockWidget);

	// Add an auto-hide dock widget
	void addDockWidget(Qt::DockWidgetArea area, NXDockWidget* dockWidget, Qt::Orientation orientation);

	NXDockWidgetTabBar* getDockWidgetBar(Qt::DockWidgetArea area);

	QList<NXDockWidget*> getDockWidgetListAtArea(Qt::DockWidgetArea area);

	void removeDockWidget(NXDockWidget* dockWidget);

protected:
	virtual bool event(QEvent* event) override;

	void setupUi();
	void initDockWidgets();
	void initUiStyles();

public slots:
	// Progress bar loading done
	void loadingDone();

	// Progress bar loading next
	void loadingProgress(int percent);

	void hideDockWidget(NXDockWidget* dockWidget);

	QRect getDockWidgetsAreaRect();

	void adjustDockWidget(NXDockWidget* dockWidget);

	void createDockWidgetBar(Qt::DockWidgetArea area);

	void showDockWidget(NXDockWidget* dockWidget);

	// Turn on the AutoHide option 
	void dockWidgetPinned(NXDockWidget* dockWidget);

	// Turn off the AutoHide option 
	void dockWidgetUnpinned(NXDockWidget* dockWidget);

	// DockWidget has been docked
	void dockWidgetDocked(NXDockWidget* dockWidget);

	// DockWidget has been undocked
	void dockWidgetUndocked(NXDockWidget* dockWidget);

	void menuWindows_triggered(QAction* action);

protected:
	Ui::AtlasMainWindowClass* _ui;

	// Current active(slide out) dockwidget or null
	NXDockWidget* _dockWidget;

	// List of all created dockwidgets
	QList<NXDockWidget*> _dockWidgets;

	// List of 4 dock tabbars
	QMap<Qt::DockWidgetArea, NXDockWidgetTabBar*> _dockWidgetBar;

	QMenu* _treeWidgetMenu;

	QProgressBar* _pProgressBar = NULL;
};

#endif // ATLASMAINWINDOW_H
