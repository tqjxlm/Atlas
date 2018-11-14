#pragma once

#include "atlasmainwindow_global.h"

#include <QDockWidget>

class QPushButton;

class NXDockWidgetTabButton;
class NXMainWindow;
class NXDockWidgetTabBar;
class NXDockWidget;
class NXDockWidgetTitle;
class QVBoxLayout;

/*! Auto Hide dockwidget
*/
class ATLASMAINWINDOW_EXPORT NXDockWidget : public QDockWidget
{
	Q_OBJECT

public:
	enum class DockWidgetState
	{
		Unknown = -1,
		Docked = 0,    //! DockWidget is docked on MainWindow
		Floating = 1,  //! DockWidget is floating
		Hidden = 2,    //! DockWidget is auto hidden
		Closed = 3,    //! DockWidget is closed by button X
	};

public:
	explicit NXDockWidget(const QString& title, QWidget* parent = nullptr);
	~NXDockWidget();

public:
	QString windowTitle() const;
	void setWindowTitle(const QString& title);

	bool isMinimized() const { return (_state == DockWidgetState::Hidden); }
	bool isDocked() const { return (_state == DockWidgetState::Docked); }
	bool isFloating() const { return (_state == DockWidgetState::Floating); }

	/*! Return current area of the dockwidget
	*/
	Qt::DockWidgetArea getArea() const { return _area; }

	DockWidgetState getState() const { return _state; }
	void setState(DockWidgetState state);

	void closeDockWidget();

	void setWidget(QWidget* widget);

	void setTabifiedDocks(const QList<QDockWidget*>& dockWidgetList);
	const QList<NXDockWidget*>& getTabifiedDocks() const { return _tabifieds; }
	void clearTabifiedDocks() { _tabifieds.clear(); }

	QAction* getMenuAction() const { return _menuAction; }
	void setMenuAction(QAction* action) { _menuAction = action;}

private:
	void openTitleMenu();
	void autoHideStateToggle();
	void updateDockLocation(Qt::DockWidgetArea area);
	void updateTopLevelState(bool topLevel);
	void removeFromTabifiedDocks(NXDockWidget* dockWidget);

protected:
	virtual bool event(QEvent* event) override;

signals:
	void signal_pinned(NXDockWidget* dockWidget);
	void signal_unpinned(NXDockWidget* dockWidget);
	void signal_docked(NXDockWidget* dockWidget);
	void signal_undocked(NXDockWidget* dockWidget);

private slots:
	void slot_menuAction() {}

private:
	Qt::DockWidgetArea _area;
	NXDockWidgetTitle* _titleWidget;
	DockWidgetState _state;
	QVBoxLayout* _layout;
	QList<NXDockWidget*> _tabifieds;
	QAction* _menuAction;
};