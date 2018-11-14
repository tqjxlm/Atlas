#include "AddModel.h"

#include <DataManager/DataManager.h>
#include <ViewerWidget/ViewerWidget.h>

#include <QDateTime>
#include <osgDB/ReadFile>
#include <osg/PositionAttitudeTransform>

#include <QIcon>
#include <QMenu>
#include <QToolButton>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>

AddModel::AddModel()
{
	_pluginName = tr("Model");
	_pluginCategory = "Data";
}

AddModel::~AddModel(void)
{
}

bool AddModel::loadModel(std::string filePath)
{
	_filepath = QString::fromStdString(filePath);
	_modelFile = osgDB::readNodeFile(filePath);
	if (!_modelFile.valid())
		return false;
	
	return true;
}

void AddModel::showNewModel()
{
	_pat = new osg::PositionAttitudeTransform;
	_pat->setPosition(_anchoredWorldPos);
	_pat->addChild(_modelFile.get());
	_currentAnchor->addChild(_pat);
}

void AddModel::setupUi(QToolBar* toolBar, QMenu* menu)
{
	// Init actions
	_modelFromFileAction = new QAction(_mainWindow);
	_modelFromFileAction->setObjectName(QStringLiteral("addModFromFileAction"));
	_modelFromFileAction->setCheckable(false);
	_modelFromFileAction->setChecked(false);
	_modelFromFileAction->setText(tr("Load Model From File"));
	QIcon icon24;
	icon24.addFile(QStringLiteral("resources/icons/model.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelFromFileAction->setIcon(icon24);
	_modelAction1 = new QAction(_mainWindow);
	_modelAction1->setObjectName(QStringLiteral("addSampleMod1Action"));
	_modelAction1->setCheckable(true);
	_modelAction1->setText(tr("Tree"));
	QIcon icon25;
	icon25.addFile(QStringLiteral("resources/icons/tree.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelAction1->setIcon(icon25);
	_modelAction2 = new QAction(_mainWindow);
	_modelAction2->setObjectName(QStringLiteral("addSampleMod2Action"));
	_modelAction2->setCheckable(true);
	_modelAction2->setText(tr("Cow"));
	QIcon icon26;
	icon26.addFile(QStringLiteral("resources/icons/cow.png"), QSize(), QIcon::Normal, QIcon::Off);
	_modelAction2->setIcon(icon26);
	_modelAction3 = new QAction(_mainWindow);
	_modelAction3->setObjectName(QStringLiteral("addSampleMod3Action"));
	_modelAction3->setCheckable(true);
	_modelAction3->setText(tr("Mark"));
	QIcon icon27;
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

	// Init button
	QIcon iconModel;
	iconModel.addFile(QString::fromUtf8("resources/icons/model.png"), QSize(), QIcon::Normal, QIcon::Off);

	QToolButton *addModelBt = new QToolButton(static_cast<QWidget*>(parent()));
	addModelBt->setIcon(iconModel);
	addModelBt->setText(tr("Model"));
	addModelBt->setToolTip(tr("Add Single Model"));
	addModelBt->setPopupMode(QToolButton::InstantPopup);
	addModelBt->setCheckable(true);

	// Group actions in a menu
	QMenu *addModelMenu = new QMenu(addModelBt);
	QMenu *addSampleMenu = new QMenu(addModelMenu);
	QIcon iconModelSample;
	iconModelSample.addFile(QString::fromUtf8("resources/icons/model.png"), QSize(), QIcon::Normal, QIcon::Off);
	addSampleMenu->setIcon(iconModelSample);
	addSampleMenu->setTitle(tr("Add Default Models"));
	addSampleMenu->addAction(_modelAction1);
	addSampleMenu->addAction(_modelAction2);
	addSampleMenu->addAction(_modelAction3);

	// Add actions to tool bar
	addModelMenu->addMenu(addSampleMenu);
	addModelMenu->addAction(_modelFromFileAction);
	addModelBt->setMenu(addModelMenu);
	toolBar->addWidget(addModelBt);

	// Add actions to menu bar
	menu->addMenu(addSampleMenu);
	menu->addAction(_modelFromFileAction);
	menu->addSeparator();

}

void AddModel::toggle(bool checked)
{
	if (checked == true)
	{
		QString filePath;

		QAction *actionobj = dynamic_cast<QAction*>(sender());
		QString actionName = actionobj->objectName();
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
			filePath = QFileDialog::getOpenFileName(_mainWindow, tr("Load model"), " ", tr("OSGB model(*.osgb);;obj model(*.obj);;3ds model(*.3ds);;Allfile(*.*);"));
		}

		if (filePath.isEmpty())
			return;
		if (!loadModel(filePath.toLocal8Bit().toStdString()))
			return;
		else
			showNewModel();
	}

	_activated = checked;
}

void AddModel::addModelFromDB(QString modelName,QString modelFilePath,osg::Vec3 pos,osg::Vec3 norl)
{
	bool isok = loadModel(modelFilePath.toLocal8Bit().toStdString());

	_currentAnchor->addChild(_pat.get());

	if (isok)
	{
		_pat->setPosition(pos);

		_pat->setUserValue("normal",norl);
		_pat->setUserValue("position", pos);
		_pat->setUserValue("filepath",modelFilePath.toLocal8Bit().toStdString());

		PluginInterface::recordNode(_pat,modelName);
	}
}

void AddModel::onLeftButton()
{
	
	if (_currentWorldPos.z() < -100)
		_currentWorldPos.z() = 1;

	_pat->setUserValue("position", _currentWorldPos);
	_pat->setUserValue("normal", _intersections.begin()->getWorldIntersectNormal());
	_pat->setUserValue("filepath",_filepath.toLocal8Bit().toStdString());
	_pat->setPosition(_currentWorldPos);
    
	if (_pluginRoot->getChildIndex(_pat) != -1)
		_currentAnchor->removeChild(_pat);

	 _currentAnchor->addChild(_pat);

	recordCurrent();

	showNewModel();
}

void AddModel::onRightButton()
{
	finish();
}

void AddModel::onMouseMove()
{
	if ( pointValid() )
	{		
		_pat->setPosition(_anchoredWorldPos);
	}
}

void AddModel::finish()
{
	_modelFile = NULL;
	_currentAnchor->removeChild(_pat.get());
	_activated = false;

	_modelFromFileAction->setChecked(false);
	_modelAction1->setChecked(false);
	_modelAction2->setChecked(false);
	_modelAction3->setChecked(false);
}


void AddModel::recordCurrent()
{
	_instanceCount++;
	
	int index1 = _filepath.lastIndexOf("/");
	QString strfile=_filepath.right(_filepath.length()-index1-1);
	int index2 =  strfile.lastIndexOf(".");
	QString name =strfile.left(index2);
	
	QDateTime time = QDateTime::currentDateTime();
	QString strcurrntime = time.toString("yyyyMMddhhmmsszzz"); 
	
	_modelUniqID = _pluginName + "_" + name + strcurrntime;
	_modelName = _modelUniqID;
	PluginInterface::recordNode(_pat,QString((const char *)_modelName.toLocal8Bit()));
}
