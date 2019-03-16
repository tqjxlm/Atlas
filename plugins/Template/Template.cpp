#include "Template.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QFileDialog>

#include <ViewerWidget/ViewerWidget.h>
#include <qmessagebox.h>

Template::Template()
{
  _pluginCategory = "Draw";
  _pluginName     = "Template";
}

Template::~Template()
{
}

void  Template::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
  _action->setObjectName(QStringLiteral("TemplateAction"));
  QIcon  icon19;
  icon19.addFile(QStringLiteral("resources/Template/Template.svg"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon19);
  _action->setText(tr("Template"));
  _action->setToolTip(tr("Say Hello world!"));

	toolBar->addAction(_action);
	menu->addAction(_action);

	connect(_action, SIGNAL(triggered()), this, SLOT(trigger()));
}

void  Template::trigger()
{
  QMessageBox  msgBox;

  msgBox.setText("Tamplate plugin, say hello ");
  msgBox.exec();
}
