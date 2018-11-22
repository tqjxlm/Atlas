#include "AtlasMainWindow.h"

#include <QDateTime>
#include <QProgressBar>
#include <QMessageBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QMenu>
#include <QEvent>
#include <QMenuBar>
#include <QLabel>
#include <algorithm>

#include "ui_AtlasMainWindow.h"
#include "NXDockWidget.h"
#include "NXDockWidgetTabBar.h"
#include "NXDockWidgetTabButton.h"

AtlasMainWindow::AtlasMainWindow(QWidget *parent, Qt::WindowFlags flags):
  QMainWindow(parent, flags),
  _dockWidget(nullptr)
{
}

AtlasMainWindow::~AtlasMainWindow()
{
}

void  AtlasMainWindow::setupUi()
{
	_ui = new Ui::AtlasMainWindowClass;
	_ui->setupUi(this);

	setWindowIcon(QIcon("resources/icons/atlas.png"));

    initDockWidgets();
}

void  AtlasMainWindow::loadingDone()
{
	_ui->statusBar->removeWidget(_pProgressBar);
  // _mousePicker->updateDrawOffset();
	delete _pProgressBar;
	_pProgressBar = NULL;
}

void  AtlasMainWindow::loadingProgress(int percent)
{
	if (_pProgressBar == NULL)
	{
		_pProgressBar = new QProgressBar;
		_pProgressBar->setMaximumWidth(400);
		_pProgressBar->setMaximumHeight(22);
		_pProgressBar->setRange(0, 100);
		_pProgressBar->setValue(0);
		_ui->statusBar->addPermanentWidget(_pProgressBar);
		_pProgressBar->setValue(percent);
	}
	else
	{
		_pProgressBar->setValue(percent);
	}
}

void  AtlasMainWindow::initDockWidgets()
{
	createDockWidgetBar(Qt::LeftDockWidgetArea);
	createDockWidgetBar(Qt::RightDockWidgetArea);
	createDockWidgetBar(Qt::TopDockWidgetArea);
	createDockWidgetBar(Qt::BottomDockWidgetArea);

	// Control panel
	{
    NXDockWidget *controlPanel = new NXDockWidget(tr("Control Panel"), this);
		controlPanel->setObjectName(QStringLiteral("controlPanel"));
    QSizePolicy  sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Minimum);
		sizePolicy1.setHorizontalStretch(0);
		sizePolicy1.setVerticalStretch(0);
		sizePolicy1.setHeightForWidth(controlPanel->sizePolicy().hasHeightForWidth());
		controlPanel->setSizePolicy(sizePolicy1);
		controlPanel->setMinimumSize(QSize(311, 0));
		controlPanel->setMaximumSize(QSize(524287, 100));
		controlPanel->setFloating(false);
		controlPanel->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget *dockWidgetContent = new QWidget();
		dockWidgetContent->setObjectName(QStringLiteral("controlPanelContent"));
		sizePolicy1.setHeightForWidth(dockWidgetContent->sizePolicy().hasHeightForWidth());
		dockWidgetContent->setSizePolicy(sizePolicy1);
		dockWidgetContent->setMaximumSize(QSize(16777215, 16777215));
    QVBoxLayout *verticalLayout = new QVBoxLayout(dockWidgetContent);
		verticalLayout->setSpacing(6);
		verticalLayout->setContentsMargins(11, 11, 11, 11);
		verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
		verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
		verticalLayout->setContentsMargins(0, 0, 0, 0);
    QTabWidget *tabWidget = new QTabWidget(dockWidgetContent);
		tabWidget->setObjectName(QStringLiteral("tabWidget"));
    QSizePolicy  sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Minimum);
		sizePolicy2.setHorizontalStretch(0);
		sizePolicy2.setVerticalStretch(0);
		sizePolicy2.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
		tabWidget->setSizePolicy(sizePolicy2);
		tabWidget->setMaximumSize(QSize(16777215, 16777215));
		tabWidget->setTabPosition(QTabWidget::North);

		verticalLayout->addWidget(tabWidget);

		controlPanel->setWidget(dockWidgetContent);
		addDockWidget(Qt::RightDockWidgetArea, controlPanel);
	}

	{
    NXDockWidget *attributePanel = new NXDockWidget(tr("Attributes"), this);
		attributePanel->setObjectName(QStringLiteral("attributePanel"));
    QSizePolicy  sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Expanding);
		sizePolicy3.setHorizontalStretch(0);
		sizePolicy3.setVerticalStretch(0);
		sizePolicy3.setHeightForWidth(attributePanel->sizePolicy().hasHeightForWidth());
		attributePanel->setSizePolicy(sizePolicy3);
		attributePanel->setMinimumSize(QSize(100, 0));
		attributePanel->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget *tableDockWidgetContents = new QWidget();
		tableDockWidgetContents->setObjectName(QStringLiteral("tableDockWidgetContents"));
		sizePolicy3.setHeightForWidth(tableDockWidgetContents->sizePolicy().hasHeightForWidth());
		tableDockWidgetContents->setSizePolicy(sizePolicy3);
    QVBoxLayout *verticalLayout_2 = new QVBoxLayout(tableDockWidgetContents);
		verticalLayout_2->setSpacing(6);
		verticalLayout_2->setContentsMargins(11, 11, 11, 11);
		verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
		verticalLayout_2->setSizeConstraint(QLayout::SetMaximumSize);
		verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    QTableWidget *attributeTable = new QTableWidget(tableDockWidgetContents);
		attributeTable->setObjectName(QStringLiteral("attributeTable"));
		attributeTable->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked);
		attributeTable->setAlternatingRowColors(false);
		attributeTable->horizontalHeader()->setVisible(false);
		attributeTable->horizontalHeader()->setMinimumSectionSize(0);
		attributeTable->horizontalHeader()->setStretchLastSection(true);
		attributeTable->verticalHeader()->setVisible(false);
		attributeTable->verticalHeader()->setStretchLastSection(false);

		verticalLayout_2->addWidget(attributeTable);

		attributePanel->setWidget(tableDockWidgetContents);
		addDockWidget(Qt::RightDockWidgetArea, attributePanel);
	}
}

void AtlasMainWindow::initUiStyles()
{
	// Tool bars (main entrance for all plugins)
  QVector<QToolBar *>  toolBars = {
		_ui->fileToolBar,
		_ui->drawToolBar,
		_ui->dataToolBar,
		_ui->measToolBar,
		_ui->analysisToolBar,
		_ui->effectToolBar,
		_ui->editToolBar
	};

  for (QToolBar *toolBar : toolBars)
	{
		// Set init style
		if (toolBar->orientation() == Qt::Vertical)
    {
      toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    else
    {
      toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    }

    toolBar->setIconSize(QSize(30, 30));

    for (QAction *action : toolBar->actions())
		{
			action->setStatusTip(action->toolTip());
		}

    for (auto *widget : toolBar->children())
		{
      QToolButton *button = dynamic_cast<QToolButton *>(widget);

			if (button)
			{
				if (toolBar->orientation() == Qt::Vertical)
        {
          button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }
        else
        {
          button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        }

				button->setStatusTip(button->toolTip());

        for (QAction *action : button->actions())
				{
					action->setStatusTip(action->toolTip());
				}

				button->setIconSize(QSize(30, 30));

				// QToolButton seems to limit maximum size by default
				button->setMaximumSize(QSize(1000, 1000));
			}
		}

		// When a tool bar is dragged and replaced, change its style accordingly
    connect(toolBar, &QToolBar::orientationChanged, [toolBar](Qt::Orientation orientation)
    {
			if (orientation == Qt::Vertical)
      {
        toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
      }
			else
      {
        toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      }

      for (auto *widget : toolBar->children())
			{
        QToolButton *button = dynamic_cast<QToolButton *>(widget);

        if (button)
        {
          if (orientation == Qt::Vertical)
          {
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
          }
          else
          {
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
          }
				}
			}
		});
	}

  for (auto child : children())
	{
    NXDockWidget *dock = dynamic_cast<NXDockWidget *>(child);

		if (dock)
		{
			dockWidgetUnpinned(dock);
			dock->setFixedWidth(250);
		}
	}
}

static Qt::ToolBarArea  dockAreaToToolBarArea(Qt::DockWidgetArea area)
{
	switch (area)
	{
  case Qt::LeftDockWidgetArea:

    return Qt::LeftToolBarArea;
  case Qt::RightDockWidgetArea:

    return Qt::RightToolBarArea;
  case Qt::TopDockWidgetArea:

    return Qt::TopToolBarArea;
  case Qt::BottomDockWidgetArea:

    return Qt::BottomToolBarArea;
	default:

		return Qt::ToolBarArea(0);
	}
}

void  AtlasMainWindow::createDockWidgetBar(Qt::DockWidgetArea area)
{
  if (_dockWidgetBar.find(area) != _dockWidgetBar.end())
  {
		return;
	}

  NXDockWidgetTabBar *dockWidgetBar = new NXDockWidgetTabBar(area);
	_dockWidgetBar[area] = dockWidgetBar;
	connect(dockWidgetBar, &NXDockWidgetTabBar::signal_dockWidgetButton_clicked, this, &AtlasMainWindow::showDockWidget);

	addToolBar(dockAreaToToolBarArea(area), dockWidgetBar);
}

void  AtlasMainWindow::dockWidgetUnpinned(NXDockWidget *dockWidget)
{
	if (dockWidget == nullptr)
  {
    return;
  }

  NXDockWidgetTabBar *dockWidgetBar = getDockWidgetBar(dockWidget->getArea());

  if (dockWidgetBar == nullptr)
  {
		return;
  }

  QList<QDockWidget *>  dockWidgetList = tabifiedDockWidgets(dockWidget);
	dockWidgetList.push_back(dockWidget);

  for (QDockWidget *qDockWidget : dockWidgetList)
	{
    NXDockWidget *dockWidget = static_cast<NXDockWidget *>(qDockWidget);

		dockWidget->setState(NXDockWidget::DockWidgetState::Hidden);

		if (!dockWidget->isHidden())
		{
			dockWidgetBar->addDockWidget(dockWidget);

			dockWidget->setTabifiedDocks(dockWidgetList);

			QMainWindow::removeDockWidget(dockWidget);
		}
	}

	if (dockWidget->getArea() == Qt::LeftDockWidgetArea)
	{
		getDockWidgetBar(Qt::TopDockWidgetArea)->insertSpacing();
		getDockWidgetBar(Qt::BottomDockWidgetArea)->insertSpacing();
	}
}

void  AtlasMainWindow::dockWidgetPinned(NXDockWidget *dockWidget)
{
  if (dockWidget == nullptr)
  {
		return;
  }

  NXDockWidgetTabBar *dockWidgetBar = getDockWidgetBar(dockWidget->getArea());

  if (dockWidgetBar == nullptr)
  {
		return;
  }

	_dockWidget = nullptr;

  QList<NXDockWidget *>  dockWidgetList = dockWidget->getTabifiedDocks();
	dockWidgetList.push_back(dockWidget);

  NXDockWidget *prevDockWidget = nullptr;

  for (NXDockWidget *dockWidget : dockWidgetList)
	{
		if (dockWidgetBar->removeDockWidget(dockWidget))
		{
      if (prevDockWidget == nullptr)
      {
				QMainWindow::addDockWidget(dockWidget->getArea(), dockWidget);
			}
      else
      {
				tabifyDockWidget(prevDockWidget, dockWidget);
			}

			prevDockWidget = dockWidget;

			dockWidget->setState(NXDockWidget::DockWidgetState::Docked);

			dockWidget->show();
		}
	}

	dockWidget->raise();

  if ((dockWidget->getArea() == Qt::LeftDockWidgetArea)
      && dockWidgetBar->isHidden())
	{
		getDockWidgetBar(Qt::TopDockWidgetArea)->removeSpacing();
		getDockWidgetBar(Qt::BottomDockWidgetArea)->removeSpacing();
	}
}

void  AtlasMainWindow::showDockWidget(NXDockWidget *dockWidget)
{
  if (dockWidget == nullptr)
  {
		return;
  }

	if (dockWidget->isHidden())
	{
		hideDockWidget(_dockWidget);

		if (dockWidget->isFloating())
		{
			QMainWindow::addDockWidget(dockWidget->getArea(), dockWidget);
			dockWidget->setFloating(false);

			QMainWindow::removeDockWidget(dockWidget);
		}

		adjustDockWidget(dockWidget);

		dockWidget->show();
		dockWidget->raise();

		dockWidget->setFocus();

		_dockWidget = dockWidget;
	}
	else
	{
		hideDockWidget(dockWidget);
	}
}

bool  AtlasMainWindow::event(QEvent *event)
{
	if (event->type() == QEvent::Resize)
	{
		hideDockWidget(_dockWidget);
	}

	// Make sure the rest of events are handled
	return QMainWindow::event(event);
}

void  AtlasMainWindow::adjustDockWidget(NXDockWidget *dockWidget)
{
	if (dockWidget == nullptr)
  {
    return;
  }

  QRect  rect = getDockWidgetsAreaRect();

	switch (dockWidget->getArea())
	{
	case Qt::LeftDockWidgetArea:
		dockWidget->setGeometry(rect.left(), rect.top(), dockWidget->width(), rect.height());
		break;
	case Qt::TopDockWidgetArea:
		dockWidget->setGeometry(rect.left(), rect.top(), rect.width(), dockWidget->height());
		break;
	case Qt::RightDockWidgetArea:
		dockWidget->setGeometry(rect.left() + rect.width() - dockWidget->width(), rect.top(), dockWidget->width(), rect.height());
		break;
	case Qt::BottomDockWidgetArea:
		dockWidget->setGeometry(rect.left(), rect.top() + rect.height() - dockWidget->height(), rect.width(), dockWidget->height());
		break;
	}
}

NXDockWidgetTabBar * AtlasMainWindow::getDockWidgetBar(Qt::DockWidgetArea area)
{
	assert(_dockWidgetBar.find(area) != _dockWidgetBar.end());

  auto  it = _dockWidgetBar.find(area);

  if (it != _dockWidgetBar.end())
  {
		return *it;
	}

	return nullptr;
}

void  AtlasMainWindow::addDockWidget(Qt::DockWidgetArea area, NXDockWidget *dockWidget)
{
	addDockWidget(area, dockWidget, Qt::Vertical);
}

void  AtlasMainWindow::addDockWidget(Qt::DockWidgetArea area, NXDockWidget *dockWidget, Qt::Orientation orientation)
{
	if (dockWidget == nullptr)
  {
    return;
  }

	connect(dockWidget, &NXDockWidget::signal_pinned, this, &AtlasMainWindow::dockWidgetPinned);
	connect(dockWidget, &NXDockWidget::signal_unpinned, this, &AtlasMainWindow::dockWidgetUnpinned);
	connect(dockWidget, &NXDockWidget::signal_docked, this, &AtlasMainWindow::dockWidgetDocked);
	connect(dockWidget, &NXDockWidget::signal_undocked, this, &AtlasMainWindow::dockWidgetUndocked);

	_dockWidgets.push_back(dockWidget);

	QMainWindow::addDockWidget(area, dockWidget, orientation);
}

void  AtlasMainWindow::removeDockWidget(NXDockWidget *dockWidget)
{
	if (dockWidget == nullptr)
  {
    return;
  }

	if (_dockWidgets.indexOf(dockWidget) < 0)
  {
    return;
  }

	_dockWidgets.removeOne(dockWidget);

  if (dockWidget->isMinimized())
  {
		dockWidgetPinned(dockWidget);
	}

	QMainWindow::removeDockWidget(dockWidget);

	dockWidget->setParent(nullptr);
}

void  AtlasMainWindow::dockWidgetDocked(NXDockWidget *dockWidget)
{
	if (dockWidget == nullptr)
  {
    return;
  }
}

void  AtlasMainWindow::dockWidgetUndocked(NXDockWidget *dockWidget)
{
	hideDockWidget(_dockWidget);

  NXDockWidgetTabBar *dockWidgetBar = getDockWidgetBar(dockWidget->getArea());

	if (dockWidgetBar == nullptr)
  {
    return;
  }

	dockWidget->clearTabifiedDocks();

	if (dockWidgetBar->removeDockWidget(dockWidget))
	{
    if (!dockWidget->isFloating())
    {
			QMainWindow::addDockWidget(dockWidget->getArea(), dockWidget);
		}

    if ((dockWidget->getArea() == Qt::LeftDockWidgetArea)
        && dockWidgetBar->isHidden())
		{
			getDockWidgetBar(Qt::TopDockWidgetArea)->removeSpacing();
			getDockWidgetBar(Qt::BottomDockWidgetArea)->removeSpacing();
		}

		dockWidget->show();
	}
}

QList<NXDockWidget *>  AtlasMainWindow::getDockWidgetListAtArea(Qt::DockWidgetArea area)
{
  QList<NXDockWidget *>  dockWidgetList;

  for (NXDockWidget *dockWidget : _dockWidgets)
	{
    if ((dockWidget->getArea() == area) && (dockWidget->isDocked()))
    {
			dockWidgetList.push_back(dockWidget);
    }
  }

	return dockWidgetList;
}

QRect  AtlasMainWindow::getDockWidgetsAreaRect()
{
  int  left = centralWidget()->x();

  QList<NXDockWidget *>  leftAreaDockWidgets = getDockWidgetListAtArea(Qt::LeftDockWidgetArea);

  for (const NXDockWidget *dockWidget : leftAreaDockWidgets)
	{
    if ((dockWidget->x() >= 0) && (dockWidget->width() > 0))
    {
			left = std::min(left, dockWidget->x());
		}
	}

  int                    top                = centralWidget()->y();
  QList<NXDockWidget *>  topAreaDockWidgets = getDockWidgetListAtArea(Qt::TopDockWidgetArea);

  for (const NXDockWidget *dockWidget : topAreaDockWidgets)
	{
    if ((dockWidget->y() >= 0) && (dockWidget->height() > 0))
    {
			top = std::min(top, dockWidget->y());
		}
	}

  int                    right                = centralWidget()->x() + centralWidget()->width();
  QList<NXDockWidget *>  rightAreaDockWidgets = getDockWidgetListAtArea(Qt::RightDockWidgetArea);

  for (const NXDockWidget *dockWidget : rightAreaDockWidgets)
	{
    if ((dockWidget->x() >= 0) && (dockWidget->width() > 0))
    {
			right = std::max(right, dockWidget->x() + dockWidget->width());
		}
	}

  int                    bottom                = centralWidget()->y() + centralWidget()->height();
  QList<NXDockWidget *>  bottomAreaDockWidgets = getDockWidgetListAtArea(Qt::BottomDockWidgetArea);

  for (const NXDockWidget *dockWidget : bottomAreaDockWidgets)
	{
    if ((dockWidget->y() >= 0) && (dockWidget->height() > 0))
    {
			bottom = std::max(bottom, dockWidget->y() + dockWidget->height());
		}
	}

	return QRect(left, top, right - left, bottom - top);
}

void  AtlasMainWindow::hideDockWidget(NXDockWidget *dockWidget)
{
  if ((dockWidget == nullptr) || (dockWidget->isHidden()))
  {
		return;
	}

	_dockWidget = nullptr;

	dockWidget->hide();
}

void  AtlasMainWindow::menuWindows_triggered(QAction *action)
{
  auto  it = _dockWidgets.begin();

	for (; it != _dockWidgets.end(); it++)
	{
		if ((*it)->getMenuAction() == action)
    {
      break;
    }
  }

	if (it == _dockWidgets.end())
  {
    return;
  }

  NXDockWidget *dockWidget = *it;

	if (dockWidget->isHidden())
	{
		hideDockWidget(_dockWidget);

		dockWidget->show();
		dockWidget->raise();
	}
	else if (dockWidget->isMinimized())
	{
		showDockWidget(dockWidget);
	}

	dockWidget->setFocus();
}
