#include "NXDockWidgetTabBar.h"

#include <QLayout>

#include "NXDockWidgetTabButton.h"
#include "NXDockWidget.h"

NXDockWidgetTabBar::NXDockWidgetTabBar(Qt::DockWidgetArea area):
  _area(area)
{
	setObjectName("DockWidgetBar");

	setFloatable(false);
	setMovable(false);

	setContextMenuPolicy(Qt::PreventContextMenu);

	setOrientation(areaToOrientation(_area));

	layout()->setSpacing(0);
	layout()->setMargin(0);

  if (orientation() == Qt::Horizontal)
	{
		_spacer = new QWidget();
		_spacer->setFixedWidth(0);
		addWidget(_spacer);
	}

	hide();
}

NXDockWidgetTabBar::~NXDockWidgetTabBar()
{
}

void  NXDockWidgetTabBar::insertSpacing()
{
  if (_spacer != nullptr)
  {
		_spacer->setFixedWidth(26);
	}
}

void  NXDockWidgetTabBar::removeSpacing()
{
  if (_spacer != nullptr)
  {
		_spacer->setFixedWidth(0);
	}
}

void  NXDockWidgetTabBar::addDockWidget(NXDockWidget *dockWidget)
{
  if (dockWidget == nullptr)
  {
		return;
	}

  NXDockWidgetTabButton *dockWidgetTabButton = new NXDockWidgetTabButton(dockWidget->windowTitle(), dockWidget->getArea());

	connect(dockWidgetTabButton, &QPushButton::toggled, this, &NXDockWidgetTabBar::dockWidgetButton_clicked);

	_tabs[dockWidgetTabButton] = dockWidget;

  QAction *action = addWidget(dockWidgetTabButton);
	dockWidgetTabButton->setAction(action);

  if (_tabs.size() == 1)
  {
    show();
	}
}

bool  NXDockWidgetTabBar::removeDockWidget(NXDockWidget *dockWidget)
{
  if (dockWidget == nullptr)
  {
		return false;
	}

  NXDockWidgetTabButton *dockWidgetTabButton = _tabs.key(dockWidget);

	if (!dockWidgetTabButton)
	{
		return false;
	}

	_tabs.remove(dockWidgetTabButton);

	removeAction(dockWidgetTabButton->getAction());

  if (_tabs.empty())
  {
		hide();
	}

	return true;
}

void  NXDockWidgetTabBar::dockWidgetButton_clicked(bool checked)
{
  NXDockWidgetTabButton *dockWidgetTabButton = dynamic_cast<NXDockWidgetTabButton *>(sender());

  if (dockWidgetTabButton == nullptr)
  {
		return;
	}

	if (checked)
	{
    for (auto button : _tabs.keys())
		{
			if (button != dockWidgetTabButton)
      {
        button->setChecked(false);
      }
    }
	}

  auto  it = _tabs.find(dockWidgetTabButton);

  if (it == _tabs.end())
  {
		return;
	}

  emit  signal_dockWidgetButton_clicked(*it);
}
