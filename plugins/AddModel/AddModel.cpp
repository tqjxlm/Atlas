#include "AddModel.h"

#include <QDateTime>
#include <osgDB/ReadFile>
#include <osg/PositionAttitudeTransform>

#include <QIcon>
#include <QMenu>
#include <QToolButton>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

#include <osgEarthAnnotation/ModelNode>
#include <osgEarthSymbology/GeometryFactory>
using namespace osgEarth::Annotation;

AddModel::AddModel()
{
	_pluginName     = tr("Model");
	_pluginCategory = "Data";
}

AddModel::~AddModel(void)
{
}

bool  AddModel::loadModel(std::string filePath)
{
	osg::ref_ptr<osg::Node>  node = osgDB::readRefNodeFile(filePath);

	if (!node)
	{
		return false;
	}

	_modelNode = new osg::PositionAttitudeTransform;
	_modelNode->addChild(node);
	_drawRoot->addChild(_modelNode);

	return true;
}

void  AddModel::setupUi(QToolBar *toolBar, QMenu *menu)
{
	// Init actions
	_modelFromFileAction = new QAction(_mainWindow);
	_modelFromFileAction->setObjectName(QStringLiteral("addModFromFileAction"));
	_modelFromFileAction->setCheckable(false);
	_modelFromFileAction->setChecked(false);
	_modelFromFileAction->setText(tr("Load Model From File"));
	QIcon  icon24;
	icon24.addFile(QStringLiteral("resources/icons/model.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelFromFileAction->setIcon(icon24);
	_modelAction1 = new QAction(_mainWindow);
	_modelAction1->setObjectName(QStringLiteral("addSampleMod1Action"));
	_modelAction1->setCheckable(true);
	_modelAction1->setText(tr("Tree"));
	QIcon  icon25;
	icon25.addFile(QStringLiteral("resources/icons/tree.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelAction1->setIcon(icon25);
	_modelAction2 = new QAction(_mainWindow);
	_modelAction2->setObjectName(QStringLiteral("addSampleMod2Action"));
	_modelAction2->setCheckable(true);
	_modelAction2->setText(tr("Cow"));
	QIcon  icon26;
	icon26.addFile(QStringLiteral("resources/icons/cow.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelAction2->setIcon(icon26);
	_modelAction3 = new QAction(_mainWindow);
	_modelAction3->setObjectName(QStringLiteral("addSampleMod3Action"));
	_modelAction3->setCheckable(true);
	_modelAction3->setText(tr("Mark"));
	QIcon  icon27;
	icon27.addFile(QStringLiteral("resources/icons/mark.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelAction3->setIcon(icon27);

	connect(_modelFromFileAction, SIGNAL(triggered()), this, SLOT(toggle()));
	connect(_modelAction1, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	connect(_modelAction2, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	connect(_modelAction3, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	registerMutexAction(_modelFromFileAction);
	registerMutexAction(_modelAction1);
	registerMutexAction(_modelAction2);
	registerMutexAction(_modelAction3);

	// Group actions in a menu
	menu = getOrAddMenu(MODEL_LAYER);

	QMenu *addSampleMenu = new QMenu(menu);
	QIcon  iconModelSample;
	iconModelSample.addFile(QString::fromUtf8("resources/icons/model.png"), QSize(), QIcon::Normal, QIcon::Off);
	addSampleMenu->setIcon(iconModelSample);
	addSampleMenu->setTitle(tr("Add Default Models"));
	addSampleMenu->addAction(_modelAction1);
	addSampleMenu->addAction(_modelAction2);
	addSampleMenu->addAction(_modelAction3);

	// Add actions to tool bar
	menu->addMenu(addSampleMenu);
	menu->addAction(_modelFromFileAction);
	menu->addSeparator();
}

void  AddModel::toggle(bool checked)
{
	if (checked == true)
	{
		QString  filePath;
		QAction *actionobj  = dynamic_cast<QAction *>(sender());
		QString  actionName = actionobj->objectName();

		if (actionName == "addSampleMod1Action")
		{
			filePath = "resources/models/tree.osgb";
		}
		else if (actionName == "addSampleMod2Action")
		{
			filePath = "resources/models/cow.osgb";
		}
		else if (actionName == "addSampleMod3Action")
		{
			filePath = "resources/models/redmark.osgb";
		}
		else if (actionName == "addModFromFileAction")
		{
			filePath = QFileDialog::getOpenFileName(_mainWindow,
			                                        tr("Load model"), " ",
			                                        tr("Supported models(*.osgb *.osg *.obj *.3ds *.ive *.dae *.fbx);;Allfile(*.*);"));
		}

		if (filePath.isEmpty())
		{
			return;
		}

		_filepath = filePath;

		if (!loadModel(filePath.toLocal8Bit().toStdString()))
		{
			QMessageBox::critical(nullptr, tr("Error"), tr("Failed to load model"));

			return;
		}
	}

	_activated = checked;
}

void  AddModel::onLeftButton()
{
	if (_activated)
	{
		// Save the temporary model to MapNode
		Style  style;
		style.getOrCreate<ModelSymbol>()->setModel(_modelNode->getChild(0));

		osg::ref_ptr<osgEarth::Annotation::ModelNode>  model;
		model = new ModelNode(_mapNode[0], style);
		model->setPosition(_currentGeoPos);

		for (std::size_t i = 0; i < MAX_SUBVIEW; i++)
		{
			_mapNode[i]->addChild(model);
		}

		recordNode(model, _filepath);
	}
}

void  AddModel::onRightButton()
{
	if (_activated)
	{
		finish();
	}
}

void  AddModel::onMouseMove()
{
	if (_activated && pointValid())
	{
		_modelNode->setPosition(_currentWorldPos);
	}
}

void  AddModel::finish()
{
	_drawRoot->removeChild(_modelNode);
	_modelNode = NULL;
	_activated = false;

	_modelFromFileAction->setChecked(false);
	_modelAction1->setChecked(false);
	_modelAction2->setChecked(false);
	_modelAction3->setChecked(false);
}
