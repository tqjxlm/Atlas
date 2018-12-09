#include "MultiView.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QToolBar>
#include <QTreeWidgetItem>

#include <osg/MatrixTransform>
#include <osgViewer/View>

#include <osgEarth/SpatialReference>

#include <ViewerWidget/ViewerWidget.h>
#include <DataManager/DataManager.h>
#include <MapController/MapController.h>

MultiView::MultiView():
  _subView(NULL),
  _subViewWidget(NULL)
{
  _pluginName     = tr("Multi View");
	_pluginCategory = "Effect";
}

MultiView::~MultiView()
{
}

void  MultiView::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("compareAction"));
	_action->setCheckable(true);
  QIcon  icon;
	icon.addFile(QStringLiteral("resources/icons/window.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon);
	_action->setText(tr("Multi View"));
	_action->setToolTip(tr("Open a Second View"));
	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	toolBar->addAction(_action);
	menu->addAction(_action);

	showInWindow1Action = new QAction(_mainWindow);
	showInWindow1Action->setObjectName(QStringLiteral("showInWindow1Action"));
	showInWindow1Action->setCheckable(true);
	showInWindow1Action->setText(tr("Window 1"));
	showInWindow1Action->setToolTip(tr("Show in window 1"));

	showInWindow2Action = new QAction(_mainWindow);
	showInWindow2Action->setObjectName(QStringLiteral("showInWindow2Action"));
	showInWindow2Action->setCheckable(true);
	showInWindow2Action->setText(tr("Window 2"));
	showInWindow2Action->setToolTip(tr("Show in window 2"));

	connect(showInWindow1Action, SIGNAL(triggered()), this, SLOT(moveToWindow()));
	connect(showInWindow2Action, SIGNAL(triggered()), this, SLOT(moveToWindow()));
}

void  MultiView::loadContextMenu(QMenu *contextMenu, QTreeWidgetItem *selectedItem)
{
  int  mask = _dataManager->getMask(selectedItem->text(0));

	showInWindow1Action->setChecked((mask & SHOW_IN_WINDOW_1) != 0x00000000);
	showInWindow2Action->setChecked((mask & SHOW_IN_WINDOW_1 << 1) != 0x00000000);
	contextMenu->addAction(showInWindow1Action);
	contextMenu->addAction(showInWindow2Action);
}

void  MultiView::toggle(bool checked)
{
	_mainViewer->stopRendering();

	if (checked)
	{
#ifdef SINGLE_VIEW

		if (!subCamera.valid())
		{
			subCamera = new osg::Camera;
		}

    osg::DisplaySettings                       *ds     = osg::DisplaySettings::instance().get();
    osg::ref_ptr<osg::GraphicsContext::Traits>  traits = new osg::GraphicsContext::Traits;
		traits->windowDecoration = false;
    traits->x                = 0;
    traits->y                = 0;
    traits->width            = 1280;
    traits->height           = 1024;
    traits->doubleBuffer     = true;
    traits->alpha            = ds->getMinimumNumAlphaBits();
    traits->stencil          = ds->getMinimumNumStencilBits();
    traits->sampleBuffers    = ds->getMultiSamples();
    traits->samples          = ds->getNumMultiSamples();
    traits->sharedContext    = _mainViewerWidget->getMainContext();

    osg::ref_ptr<osgQt::GraphicsWindowQt>  gw = new osgQt::GraphicsWindowQt(traits.get());
		subCamera->setGraphicsContext(gw);
		subCamera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));

    GLuint  buffer = gw->getTraits()->doubleBuffer ? GL_BACK : GL_FRONT;
		subCamera->setReadBuffer(buffer);
		subCamera->setDrawBuffer(buffer);
		subCamera->setProjectionResizePolicy(osg::Camera::VERTICAL);

		subCamera->setCullMask(0xfffffff1);

		_mainViewerWidget->getMainView()->addSlave(subCamera, osg::Matrix::scale((double)traits->width / traits->height, 1.0, 1.0), osg::Matrixd());

		mainViewerWidget->startThreading();

		subWidget = gw->getGLWidget();
		mainViewerWidget->addWidgetToLayout(subWidget, 0, 1);

#else

		if (!_subViewWidget)
    {
      initSubView();
    }

    _mainViewer->setWidgetInLayout(_subViewWidget, 0, 1);
#endif
	}
	else
	{
#ifdef SINGLE_VIEW
		mainViewerWidget->getMainView()->removeSlave(0);
#else
		_subViewWidget->hide();
#endif
	}

	_mainViewer->startRendering();
}

void  MultiView::initSubView()
{
	_subViewWidget = _mainViewer->createViewWidget(_mainViewer->createGraphicsWindow(0, 0, 1280, 1024, "Window2", true), _root);
  _subView       = _mainViewer->getView(_mainViewer->getNumViews() - 1);
	_subView->getCamera()->setCullMask(SHOW_IN_WINDOW_1 << 1);

  _subView->setCameraManipulator(_mainViewer->getMainView()->getCameraManipulator(), false);
  
  MapController* manipulator = dynamic_cast<MapController*>(_subView->getCameraManipulator());
  if (manipulator)
    manipulator->registerWithView(_subView, 1);
}

void  MultiView::moveToWindow()
{
  QList<QTreeWidgetItem *>  itemList = _dataManager->getSelectedItems();
  int mask = (showInWindow1Action->isChecked() ? SHOW_IN_WINDOW_1 : 0)
              | (showInWindow2Action->isChecked() ? SHOW_IN_WINDOW_1 << 1 : 0);

  for(auto item : itemList)
	{
    QTreeWidgetItem *parent   = item->parent();
    auto             nodeName = item->text(0);

		_dataManager->setWindowMask(nodeName, mask);
	}
}
