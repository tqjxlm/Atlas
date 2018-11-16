﻿#include "OrthoMap.h"

#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QToolBar>
#include <QToolButton>

#include <osgSim/OverlayNode>

#include "SaveOrthoProjDialog.h"
#include <DataManager/FindNode.hpp>

OrthoMap::OrthoMap()
    : _selectedNode(nullptr)
{
	_pluginName = tr("Orthographic Map");
	_pluginCategory = "Edit";
}

OrthoMap::~OrthoMap()
{
}

void OrthoMap::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_orthoModelAction = new QAction(_mainWindow);
	_orthoModelAction->setObjectName(QStringLiteral("_orthoModelAction"));
	_orthoModelAction->setCheckable(true);
	_orthoModelAction->setText(tr("DOM for Current Model"));
	connect(_orthoModelAction, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	_orthoAreaAction = new QAction(_mainWindow);
	_orthoAreaAction->setObjectName(QStringLiteral("_orthoAreaAction"));
	_orthoAreaAction->setCheckable(true);
	_orthoAreaAction->setEnabled(false);
	_orthoAreaAction->setText(tr("DOM for Selected Area"));
	connect(_orthoAreaAction, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	_orthoModelDSMAction = new QAction(_mainWindow);
	_orthoModelDSMAction->setObjectName(QStringLiteral("_orthoModelDSMAction"));
	_orthoModelDSMAction->setCheckable(true);
	_orthoModelDSMAction->setText(tr("DSM for Current Model"));
	connect(_orthoModelDSMAction, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	_orthoAreaDSMAction = new QAction(_mainWindow);
	_orthoAreaDSMAction->setObjectName(QStringLiteral("_orthoAreaDSMAction"));
	_orthoAreaDSMAction->setCheckable(true);
	_orthoAreaDSMAction->setEnabled(false);
	_orthoAreaDSMAction->setText(tr("DSM for Selected Area"));
	connect(_orthoAreaDSMAction, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));


	registerMutexAction(_orthoAreaAction);
	registerMutexAction(_orthoAreaDSMAction);

	QToolButton *orthoButton = new QToolButton(_mainWindow);
	QIcon orthoicon;
	orthoicon.addFile(QString::fromUtf8("resources/icons/satellite.png"), QSize(), QIcon::Normal, QIcon::Off);
	orthoButton->setIcon(orthoicon);
	orthoButton->setText(tr("Ortho"));
	orthoButton->setToolTip(tr("Generate Orthographic Image"));
	orthoButton->setPopupMode(QToolButton::InstantPopup);
	QMenu *orthoMenu = new QMenu(orthoButton);
	orthoMenu->setIcon(orthoicon);
	orthoMenu->addAction(_orthoModelAction);
	orthoMenu->addAction(_orthoAreaAction);
	orthoMenu->addAction(_orthoModelDSMAction);
	orthoMenu->addAction(_orthoAreaDSMAction);
	orthoMenu->setTitle(tr("Orthographics"));
	orthoButton->setMenu(orthoMenu);

	toolBar->addWidget(orthoButton);
	menu->addMenu(orthoMenu);
}

void OrthoMap::onDoubleClick()
{
	setupWorkingDialog(_overlayNode);
}

void OrthoMap::onRightButton()
{
	_saveDialog->finish();
	delete _saveDialog;
	findNodeInNode("Sky", _root)->setNodeMask(1);
}

void OrthoMap::toggle(bool checked)
{
	QAction* action = dynamic_cast<QAction*>(sender());

	// In area mode, the actions should be toggled until the area is drawn
	if (action == _orthoAreaDSMAction)
	{
		_mode = SaveOrthoProjDialog::DSM;
		_activated = checked;
	}
	else if (action == _orthoAreaAction)
	{
		_mode = SaveOrthoProjDialog::DOM;
		_activated = checked;
	}
	// In model mode, the actions act like a trigger, so they should be toggled off immediately
	else if (checked)
	{
		if (action == _orthoModelAction)
			_mode = SaveOrthoProjDialog::DOM;
		else if (action == _orthoModelDSMAction)
			_mode = SaveOrthoProjDialog::DSM;
		else
			QMessageBox::critical(NULL, tr("Error"), tr("OrthoMap plugin received from invalid sender!"));

		if (_selectedNode == NULL)
			QMessageBox::critical(NULL, tr("Error"), tr("No active model selected!"));
		else
			setupWorkingDialog(_selectedNode);

		action->toggle();
	}

}

void OrthoMap::setupWorkingDialog(osg::Node* scene)
{
	_saveDialog = new SaveOrthoProjDialog(*_mainViewer, scene, _globalWKT, _mode, 0);

	connect(_saveDialog, SIGNAL(accepted()), this, SLOT(onRightButton()));
	connect(_saveDialog, SIGNAL(rejected()), this, SLOT(onRightButton()));
	findNodeInNode("Sky", _root)->setNodeMask(0);
	_saveDialog->setup();
}
