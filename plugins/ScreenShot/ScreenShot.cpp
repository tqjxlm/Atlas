#include "ScreenShot.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QFileDialog>

#include <ViewerWidget/ViewerWidget.h>
#include "CaptureImageCallback.h"

ScreenShot::ScreenShot()
{
	_pluginCategory = "Effect";
	_pluginName = "Screen Shot";
}

ScreenShot::~ScreenShot()
{

}

void ScreenShot::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("screenShotAction"));
	QIcon icon19;
	icon19.addFile(QStringLiteral("resources/icons/screenshot.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon19);
	_action->setText(tr("ScreenShot"));
	_action->setToolTip(tr("Save current scene screen"));

	toolBar->addAction(_action);
	menu->addAction(_action);

	connect(_action, SIGNAL(triggered()), this, SLOT(trigger()));
}

void ScreenShot::trigger()
{
	QString saveScreenshotFileName = QFileDialog::getSaveFileName((QWidget*)parent(), tr("save file"), "", tr("JPEG file(*.jpg);;Allfile(*.*)"));
	if (saveScreenshotFileName.isEmpty())
		return;

	GLenum buffer = _mainViewer->getMainView()->getCamera()->getGraphicsContext()->getTraits()->doubleBuffer ? GL_BACK : GL_FRONT;

	osg::ref_ptr<CaptureImageCallback> obj = new CaptureImageCallback(buffer, saveScreenshotFileName.toLocal8Bit().toStdString());

	_mainViewer->getMainView()->getCamera()->setFinalDrawCallback(obj);

	_mainViewer->renderingTraversals();
}
