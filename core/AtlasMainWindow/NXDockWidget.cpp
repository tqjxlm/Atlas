#include "NXDockWidget.h"

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListView>
#include <QPushButton>
#include <QLabel>
#include <QToolBar>
#include <QEvent>
#include <QMenu>

#include "NXDockWidgetTabButton.h"
#include "NXDockWidgetTitle.h"

NXDockWidget::NXDockWidget(const QString &title, QWidget *parent):
  QDockWidget(parent),
  _area(Qt::NoDockWidgetArea),
  _state(DockWidgetState::Unknown)
{
	setObjectName("DockWidget");

	setAutoFillBackground(true);

	_titleWidget = new NXDockWidgetTitle();
	setWindowTitle(title);

	setTitleBarWidget(_titleWidget);


	_layout = new QVBoxLayout;
	_layout->setContentsMargins(0, 2, 0, 0);

  QWidget *widget = new QWidget();
	widget->setLayout(_layout);
	QDockWidget::setWidget(widget);

	connect(_titleWidget, &NXDockWidgetTitle::menuButton_pressed, this, &NXDockWidget::openTitleMenu);
	connect(_titleWidget, &NXDockWidgetTitle::autoHideButton_pressed, this, &NXDockWidget::autoHideStateToggle);
	connect(_titleWidget, &NXDockWidgetTitle::closeButton_pressed, this, &NXDockWidget::closeDockWidget);

	connect(this, &QDockWidget::dockLocationChanged, this, &NXDockWidget::updateDockLocation);
	connect(this, &QDockWidget::topLevelChanged, this, &NXDockWidget::updateTopLevelState);
}

NXDockWidget::~NXDockWidget()
{
}

QString  NXDockWidget::windowTitle() const
{
	return _titleWidget->getText();
}

void  NXDockWidget::setWindowTitle(const QString &text)
{
  QString  title = text.isEmpty() ? "Undefined" : text;

	_titleWidget->setText(title);
	QDockWidget::setWindowTitle(title);
}

void  NXDockWidget::closeDockWidget()
{
  if (isMinimized())
  {
    emit  signal_pinned(this);
	}

	setState(DockWidgetState::Closed);

	hide();
}

void  NXDockWidget::openTitleMenu()
{
	// TODO add menu here

  QMenu  menu;

	menu.addAction("Float", this, SLOT(slot_menuAction()));
	menu.addAction("Dock", this, SLOT(slot_menuAction()));
	menu.addAction("Auto Hide", this, SLOT(slot_menuAction()));
	menu.addAction("Hide", this, SLOT(slot_menuAction()));

	menu.exec(_titleWidget->menuPos());
}

void  NXDockWidget::autoHideStateToggle()
{
  if (isMinimized())
	{
		setState(DockWidgetState::Docked);
    emit  signal_pinned(this);
	}
  else
	{
		setState(DockWidgetState::Hidden);
    emit  signal_unpinned(this);
	}
}

void  NXDockWidget::updateDockLocation(Qt::DockWidgetArea area)
{
	_area = area;

  if (_area != Qt::NoDockWidgetArea)
  {
		updateTopLevelState(false);
	}
}

void  NXDockWidget::updateTopLevelState(bool topLevel)
{
	_titleWidget->setAutoHideEnadled(false);

  if (topLevel)
	{
		setState(DockWidgetState::Floating);

    for (auto dockWidget : _tabifieds)
		{
			dockWidget->removeFromTabifiedDocks(this);
    }

		clearTabifiedDocks();

    emit  signal_undocked(this);
	}
	else
	{
		setState(DockWidgetState::Docked);

    QList<QDockWidget *>  tabifiedDockWidgetList = static_cast<QMainWindow *>(parentWidget())->tabifiedDockWidgets(this);
		tabifiedDockWidgetList.push_back(this);

    for (auto qDockWidget : tabifiedDockWidgetList)
		{
      qobject_cast<NXDockWidget *>(qDockWidget)->setTabifiedDocks(tabifiedDockWidgetList);
		}

    emit  signal_docked(this);
	}
}

void  NXDockWidget::setState(DockWidgetState state)
{
	_state = state;

  switch (state)
	{
  case DockWidgetState::Docked:
    _titleWidget->setFloating(true);
    break;
  case DockWidgetState::Floating:
    _titleWidget->setFloating(false);
    break;
  case DockWidgetState::Hidden:
    _titleWidget->setAutoHideEnadled(true);
    break;
  case DockWidgetState::Closed:
    break;
  default:
    break;
	}
}

bool  NXDockWidget::event(QEvent *event)
{
// qDebug() << event->type();

  if (event->type() == QEvent::Enter)
	{
	}
  else if (event->type() == QEvent::Leave)
	{
	}
  else if (event->type() == QEvent::FocusOut)
	{
	}

	// Make sure the rest of events are handled
	return QDockWidget::event(event);
}

void  NXDockWidget::setWidget(QWidget *widget)
{
	_layout->addWidget(widget);
}

void  NXDockWidget::setTabifiedDocks(const QList<QDockWidget *> &dockWidgetList)
{
	_tabifieds.clear();

  for (auto qDockWidget : dockWidgetList)
	{
    _tabifieds.push_back(static_cast<NXDockWidget *>(qDockWidget));
	}
}

void  NXDockWidget::removeFromTabifiedDocks(NXDockWidget *dockWidget)
{
	_tabifieds.erase(std::remove(std::begin(_tabifieds), std::end(_tabifieds), dockWidget), std::end(_tabifieds));
}
