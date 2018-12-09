#include "DataManager.h"

#include <QVector>
#include <QMenu>
#include <QMetaType>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QSignalMapper>

#include <osgSim/OverlayNode>
#include <osgEarth/GeoData>
#include <osgEarth/MapNode>

// local tools
#include <SettingsManager/SettingsManager.h>
#include <AtlasMainWindow/AtlasMainWindow.h>
#include <AtlasMainWindow/NXDockWidget.h>

#include "FindNode.hpp"
#include "DataTree.h"
//#include "FeatureStyleSettingDlg.h"
//#include "ColorVisitor.h"
//#include "FontVisitor.h"
//#include "ModelManipulator.h"

DataManager::DataManager(SettingsManager* settingsManager, QObject* parent /*= NULL*/)
	: QObject(parent)
	, _countLoadingData(0)
	, _settingsManager(settingsManager)
	//, _featureStyleDlg(NULL)
{
	initDataTree();
	setupUi();

	//_colorvisitor = new ColorVisitor;
	//_fontvisitor = new FontVisitor;
}

DataManager::~DataManager()
{
}

void DataManager::reset()
{
	_nodeTree->clear();
}

void DataManager::setupUi()
{
	AtlasMainWindow* mainWindow = (AtlasMainWindow*)parent();
	QToolBar* fileToolBar = mainWindow->findChild<QToolBar*>("fileToolBar");
	QToolBar* effectToolBar = mainWindow->findChild<QToolBar*>("effectToolBar");
	QMenu* effectMenu = mainWindow->findChild<QMenu*>("effectMenu");

	//// File actions
	//QAction* newAction = new QAction(mainWindow);
	//newAction->setObjectName(QStringLiteral("newAction"));
	//QIcon icon;
	//icon.addFile(QStringLiteral("resources/icons/new.png"), QSize(), QIcon::Normal, QIcon::Off);
	//newAction->setIcon(icon);
	//newAction->setText(tr("New"));
	//newAction->setToolTip(tr("Start new project"));

	//QAction* openPrjAction = new QAction(mainWindow);
	//openPrjAction->setObjectName(QStringLiteral("openPrjAction"));
	//QIcon icon8;
	//icon8.addFile(QStringLiteral("resources/icons/open.ico"), QSize(), QIcon::Normal, QIcon::Off);
	//openPrjAction->setIcon(icon8);
	//openPrjAction->setText(tr("Open"));
	//openPrjAction->setToolTip(tr("Open an existing project"));

	//fileToolBar->addAction(newAction);
	//fileToolBar->addAction(openPrjAction);
	//fileMenu->insertAction(exitAction, newAction);
	//fileMenu->insertAction(exitAction, openPrjAction);

	//// Save actions
	//QAction* saveAction = new QAction(mainWindow);
	//saveAction->setObjectName(QStringLiteral("saveAction"));
	//QIcon icon1;
	//icon1.addFile(QStringLiteral("resources/icons/save.ico"), QSize(), QIcon::Normal, QIcon::Off);
	//saveAction->setIcon(icon1);
	//saveAction->setText(tr("Save"));
	//saveAction->setToolTip(tr("Save project"));

	//QAction* saveAsAction = new QAction(mainWindow);
	//saveAsAction->setObjectName(QStringLiteral("saveasAction"));
	//QIcon icon2;
	//icon2.addFile(QStringLiteral("resources/icons/save_as.ico"), QSize(), QIcon::Normal, QIcon::Off);
	//saveAsAction->setIcon(icon2);
	//saveAsAction->setText(tr("Save As"));
	//saveAsAction->setToolTip(tr("Save project as a new file"));

	//QToolButton *saveButton = new QToolButton(mainWindow);
	//QIcon icon3;
	//icon3.addFile(QString::fromUtf8("resources/icons/save.png"), QSize(), QIcon::Normal, QIcon::Off);
	//saveButton->setIcon(icon3);
	//saveButton->setText(tr("Save"));
	//saveButton->setToolTip(tr("Save Project"));
	//saveButton->setPopupMode(QToolButton::MenuButtonPopup);
	//QMenu *saveMenu = new QMenu(saveButton);
	//saveMenu->addAction(saveAction);
	//saveMenu->addAction(saveAsAction);
	//saveButton->setMenu(saveMenu);
	//saveButton->setDefaultAction(saveAction);

	//fileToolBar->addWidget(saveButton);
	//fileMenu->insertAction(exitAction, saveAction);
	//fileMenu->insertAction(exitAction, saveAsAction);
	//fileMenu->insertSeparator(exitAction);

	// Effect actions
	QAction* resetViewAction = new QAction(mainWindow);
	resetViewAction->setObjectName(QStringLiteral("resetViewAction"));
	QIcon icon6;
	icon6.addFile(QStringLiteral("resources/icons/show_all.png"), QSize(), QIcon::Normal, QIcon::Off);
	resetViewAction->setIcon(icon6);
	resetViewAction->setText(tr("View"));
	resetViewAction->setToolTip(tr("Reset to full view"));

	effectToolBar->addAction(resetViewAction);
	effectMenu->addAction(resetViewAction);

	// Context menu actions
	//showAttributeTableAction = new QAction(mainWindow);
	//showAttributeTableAction->setObjectName(QStringLiteral("showAttributeTableAction"));
	//showAttributeTableAction->setText(tr("Attributes"));
	//showAttributeTableAction->setToolTip(tr("Show attribute table"));

	//showMetatDataAction = new QAction(mainWindow);
	//showMetatDataAction->setObjectName(QStringLiteral("showMetatDataAction"));
	//showMetatDataAction->setText(tr("Meta Data"));
	//showMetatDataAction->setToolTip(tr("Show meta data"));

	showAllChildrenAction = new QAction(mainWindow);
	showAllChildrenAction->setObjectName(QStringLiteral("showAllChildrenAction"));
	showAllChildrenAction->setText(tr("Show"));
	showAllChildrenAction->setToolTip(tr("Show all children"));

	hideAllChildrenAction = new QAction(mainWindow);
	hideAllChildrenAction->setObjectName(QStringLiteral("hideAllChildrenAction"));
	hideAllChildrenAction->setText(tr("Hide"));
	hideAllChildrenAction->setToolTip(tr("Hide all children"));

	saveNodeAction = new QAction(mainWindow);
	saveNodeAction->setObjectName(QStringLiteral("saveNodeAction"));
	saveNodeAction->setText(tr("Save"));
	saveNodeAction->setToolTip(tr("Save node"));

	deleteNodeAction = new QAction(mainWindow);
	deleteNodeAction->setObjectName(QStringLiteral("deleteNodeAction"));
	deleteNodeAction->setText(tr("Delete"));
	deleteNodeAction->setToolTip(tr("Delete node"));

	//showNodeLabelAction = new QAction(mainWindow);
	//showNodeLabelAction->setObjectName(QStringLiteral("showNodeLabelAction"));
	//showNodeLabelAction->setText(tr("Show Labels"));
	//showNodeLabelAction->setToolTip(tr("Show or hide feature labels"));

	//changeLayerStyleAction = new QAction(mainWindow);
	//changeLayerStyleAction->setObjectName(QStringLiteral("changeLayerStyleAction"));
	//changeLayerStyleAction->setText(tr("Style"));
	//changeLayerStyleAction->setToolTip(tr("Change feature style"));

	//moveModelAction = new QAction(mainWindow);
	//moveModelAction->setObjectName(QStringLiteral("moveModelAction"));
	//moveModelAction->setCheckable(true);
	//QIcon icon12;
	//icon12.addFile(QStringLiteral("resources/icons/move.png"), QSize(), QIcon::Normal, QIcon::Off);
	//moveModelAction->setIcon(icon12);
	//moveModelAction->setText(tr("Move/Zoom"));
	//moveModelAction->setToolTip(tr("Move or zoom model"));

	//rotateModelAction = new QAction(mainWindow);
	//rotateModelAction->setObjectName(QStringLiteral("rotateModelAction"));
	//rotateModelAction->setCheckable(true);
	//QIcon icon18;
	//icon18.addFile(QStringLiteral("resources/icons/rotate.png"), QSize(), QIcon::Normal, QIcon::Off);
	//rotateModelAction->setIcon(icon18);
	//rotateModelAction->setText(tr("Rotate"));
	//rotateModelAction->setToolTip(tr("Rotate model"));

	connect(resetViewAction, SIGNAL(triggered()), this, SIGNAL(resetCamera()));
	
	//connect(showAttributeTableAction, SIGNAL(triggered()), this, SLOT(showAttributeTableSlot()));
	//connect(showMetatDataAction, SIGNAL(triggered()), this, SLOT(showMetaDataSlot()));
	//connect(changeLayerStyleAction, SIGNAL(triggered()), this, SLOT(changeLayerStyleSlot()));

	//connect(moveModelAction, SIGNAL(triggered(bool)), this, SLOT(editModelNodeSlot_move(bool)));
	//connect(rotateModelAction, SIGNAL(triggered(bool)), this, SLOT(editModelNodeSlot_spin(bool)));
	//connect(showNodeLabelAction, SIGNAL(triggered(bool)), this, SLOT(showLayerLabelSlot(bool)));

	connect(deleteNodeAction, SIGNAL(triggered()), _nodeTree, SLOT(deleteNodeSlot()));
	connect(saveNodeAction, SIGNAL(triggered()), _nodeTree, SLOT(saveNodeSlot()));

	{
		QSignalMapper* mapper = new QSignalMapper(this);
		connect(showAllChildrenAction, SIGNAL(triggered()), mapper, SLOT(map()));
		connect(hideAllChildrenAction, SIGNAL(triggered()), mapper, SLOT(map()));

		mapper->setMapping(showAllChildrenAction, true);
		mapper->setMapping(hideAllChildrenAction, false);

		connect(mapper, SIGNAL(mapped(int)), _nodeTree, SLOT(switchAllSlot(int)));
	}

	//connect(newAction, SIGNAL(triggered()), this, SLOT(new()));
	//connect(openPrjAction, SIGNAL(triggered()), this, SLOT(openPrj()));
	//connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));
	//connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));
}

void DataManager::initDataTree()
{
	AtlasMainWindow* mainWindow = (AtlasMainWindow*)parent();

	_treeWidgetMenu = new QMenu(mainWindow);

	// Init dock
	NXDockWidget *dataPanel = new NXDockWidget(tr("Data Panel"), mainWindow);
	QWidget *treeDockWidgetContents;
	QVBoxLayout *verticalLayout;

	dataPanel->setObjectName(QStringLiteral("dataPanel"));
	QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(dataPanel->sizePolicy().hasHeightForWidth());
	dataPanel->setSizePolicy(sizePolicy);
	dataPanel->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	treeDockWidgetContents = new QWidget();
	treeDockWidgetContents->setObjectName(QStringLiteral("treeDockWidgetContents"));
	sizePolicy.setHeightForWidth(treeDockWidgetContents->sizePolicy().hasHeightForWidth());
	treeDockWidgetContents->setSizePolicy(sizePolicy);
	verticalLayout = new QVBoxLayout(treeDockWidgetContents);
	verticalLayout->setSpacing(6);
	verticalLayout->setContentsMargins(11, 11, 11, 11);
	verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
	verticalLayout->setContentsMargins(0, 0, 0, 0);
	
	// Init tree
	_nodeTree = new DataTree(_settingsManager, treeDockWidgetContents);
	_nodeTree->setObjectName(QStringLiteral("nodeTree"));
	_nodeTree->setContextMenuPolicy(Qt::CustomContextMenu);
	_nodeTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	_nodeTree->setTabKeyNavigation(false);
	_nodeTree->setAlternatingRowColors(true);
	_nodeTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
	_nodeTree->setSelectionBehavior(QAbstractItemView::SelectRows);
	_nodeTree->setHeaderHidden(true);

	verticalLayout->addWidget(_nodeTree);
	dataPanel->setWidget(treeDockWidgetContents);
	mainWindow->addDockWidget(Qt::RightDockWidgetArea, dataPanel);

	// Tree slots
	connect(_nodeTree, SIGNAL(itemChanged(QTreeWidgetItem*, int)), _nodeTree, SLOT(switchDataSlot(QTreeWidgetItem*, int)));
	connect(_nodeTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(doubleClickTreeSlot(QTreeWidgetItem*, int)));
	connect(_nodeTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showDataTreeContextMenu(const QPoint &)));
}

void DataManager::recordData(osg::Node* node, const QString& name, const QString& parent, bool hidden)
{
	_nodeTree->addRecord(node, name, parent, hidden);
}

void DataManager::recordData(osgEarth::Layer* layer, const QString& name, const QString& parent, osgEarth::GeoExtent* extent, bool hidden)
{
	_nodeTree->addRecord(layer, name, parent, extent, hidden);
}

void DataManager::removeData(const QString& nodeName)
{
	_nodeTree->removeRecord(nodeName);
}

void DataManager::switchData(const QString& nodeName, bool checked)
{
	_nodeTree->switchRecord(nodeName, checked);
}

void DataManager::setWindowMask(const QString& nodeName, int mask)
{
	_nodeTree->setWindowMask(nodeName, mask);
}

int DataManager::getMask(const QString& nodeName)
{
	auto record = _nodeTree->getRecord(nodeName);
	if (record)
		return record->mask();
	else
		return 0x00000000;
}

void DataManager::registerDataRoots(osg::Group* root)
{
	osg::Group* mapRoot = findNodeInNode("Map Root", root)->asGroup();
  _nodeTree->_overlayNode = static_cast<osgSim::OverlayNode*>(findNodeInNode("Data Overlay", root));
	for (unsigned i = 0; i < MAX_SUBVIEW; i++)
	{
		osg::Node* map = findNodeInNode(QString("Map%1").arg(i).toStdString(), mapRoot);
		if (map)
			_nodeTree->_mainMap[i] = dynamic_cast<osgEarth::MapNode*>(map)->getMap();
		else
			_nodeTree->_mainMap[i] = NULL;
	}
}

const osgEarth::GeoExtent* DataManager::getExtent(const QString& name)
{
	DataRecord* record = _nodeTree->getRecord(name);
	return record ? record->extent() : NULL;
}

QList<QTreeWidgetItem*> DataManager::getSelectedItems()
{
	return _nodeTree->selectedItems();
}

QMap<QString, QVector<attrib>>& DataManager::getAttributeList()
{
	return _attributeLists;
}

QMap<QString, QVector<feature>>& DataManager::getFreatureTables()
{
	return _featureTables;
}

QMap<QString, QStringList>& DataManager::getFreatureFieldList()
{
	return _featureFieldList;
}

void DataManager::updateAttributeList(const QString& name, const QVector<attrib>& attribs)
{
	_attributeLists[name] = attribs;
}

void DataManager::updateFeatureTable(const QString& name, const QVector<feature>& features)
{
	_featureTables[name] = features;
}

void DataManager::updateFeatureFieldList(const QString& name, const QStringList& featureList)
{
	_featureFieldList[name] = featureList;
}

void DataManager::newProject()
{
	reset();
}

void DataManager::showMetaDataSlot()
{
	QDockWidget* tableDockWidget = ((AtlasMainWindow*)parent())->findChild<QDockWidget*>("attributePanel");
	tableDockWidget->setHidden(false);

	QList<QTreeWidgetItem *> itemList = _nodeTree->selectedItems();
	QTreeWidgetItem *item = itemList.at(0);
	auto record = _nodeTree->getRecord(item->text(0));
	if (!record)
		return;

	QTableWidget* tableWidget = ((AtlasMainWindow*)parent())->findChild<QTableWidget*>("attributeTable");

	typedef QPair<QString, QString> attrib;
	if (getAttributeList().find(item->text(0)) != getAttributeList().end())
	{
		QVector<attrib> &attribList = getAttributeList()[item->text(0)];

		tableWidget->clear();
		tableWidget->setRowCount(attribList.size());
		tableWidget->setColumnCount(2);
		for (int i = 0; i < attribList.size(); i++)
		{
			QTableWidgetItem *title = new QTableWidgetItem;
			QTableWidgetItem *value = new QTableWidgetItem;
			title->setText(attribList[i].first);
			value->setText(attribList[i].second);
			value->setToolTip(value->text());
			tableWidget->setItem(i, 0, title);
			tableWidget->setItem(i, 1, value);
			tableWidget->resizeRowsToContents();
		}
	}
}

void DataManager::doubleClickTreeSlot(QTreeWidgetItem* item, int column)
{
	// Try to get the data record
	auto record = _nodeTree->getRecord(item->text(column));
	if (!record)
		return;

	// Fly to and center on the target node based on previously calculated bounding
	if (record->extent() && record->extent()->isValid())
	{
		// When the record is an OSGEarth layer
		double xmin, xmax, ymin, ymax;
		record->extent()->getBounds(xmin, ymin, xmax, ymax);
		const osgEarth::SpatialReference* source_srs = record->extent()->getSRS();

		osgEarth::Bounds bound;
		source_srs->guessBounds(bound);

		if (ymin == bound.yMin())
			ymin += 0.00000001;
		if (ymax == bound.yMax())
			ymax -= 0.00000001;
		
		source_srs->transformExtentToMBR(_settingsManager->getGlobalSRS(), xmin, ymin, xmax, ymax);

		osg::BoundingSphere bs(osg::BoundingBox(xmin, ymin, 0, xmax, ymax, 0));

		emit moveToBounding(&bs, 0);
	}
	else if (record->bounding() && record->bounding()->valid())
	{
		// When the record is a plain OSG node
		QString str = item->text(column);
		if (record->isLayer())
			emit moveToBounding(record->bounding(), str.contains("info ") ? 1000 : 0);
		else
			emit moveToNode(record->node(), str.contains("info ") ? 1000 : 0);
	}
  else
  {
    osg::notify(osg::WARN) << "Failed to fit view: no valid bounding";
  }

}

void DataManager::showDataTreeContextMenu(const QPoint &pos)
{
	QModelIndex index = _nodeTree->indexAt(pos);

	QTreeWidgetItem* childitem = _nodeTree->itemAt(pos);

	if (!childitem)
		return;

	QString itemText = childitem->text(0);

	int childchildItemCount = childitem->childCount();

	_treeWidgetMenu->clear();

	if (childchildItemCount > 0)
	{
		_treeWidgetMenu->addAction(showAllChildrenAction);
		_treeWidgetMenu->addAction(hideAllChildrenAction);
	}
	else
	{
		emit requestContextMenu(_treeWidgetMenu, childitem);
	}

	// TODO: The type-related context actions should be handled by corresponding plugins
	//DataType dataType = getItemCategory(childitem);
	//switch (dataType)
	//{

	//case(SHAPELAYER):
	//{
	//	bool ischeck = childitem->data(0, Qt::UserRole).toBool();
	//	showNodeLabelAction->setChecked(ischeck);
	//	_treeWidgetMenu->addAction(showNodeLabelAction);
	//	_treeWidgetMenu->addAction(changeLayerStyleAction);
	//	_treeWidgetMenu->addAction(showAttributeTableAction);
	//	_treeWidgetMenu->addAction(showMetatDataAction);
	//}
	//break;
	//case(IMAGELAYER):
	//case(TERRAINLAYER):
	//	_treeWidgetMenu->addAction(showMetatDataAction);
	//	break;

	//case(MODEL):
	//{
	//	int ischeck = childitem->data(0, Qt::UserRole).toInt();
	//	moveModelAction->setChecked(ischeck == 1);
	//	_treeWidgetMenu->addAction(moveModelAction);
	//}

	//break;
	//default:
	//	break;
	//}

	_treeWidgetMenu->addSeparator();
	_treeWidgetMenu->addAction(deleteNodeAction);
	_treeWidgetMenu->addAction(saveNodeAction);

	if (index.isValid())
	{
		_treeWidgetMenu->exec(QCursor::pos());
	}
}

//void DataManager::colorChangeSlot(PrimType type, osg::Vec4 color)
//{
//	_colorvisitor->setColor(type, color);
//	_activatedNode->accept(*_colorvisitor);
//
//	QString currentNodeName = QString::fromStdString(_activatedNode->getName());
//	osg::Group *group = _activatedNode->getParent(0);
//	for (unsigned int i = 0; i < group->getNumChildren(); i++)
//	{
//		QString childname = QString::fromStdString(group->getChild(i)->getName());
//		if (childname.contains(currentNodeName) && childname != currentNodeName)
//		{
//			osg::Node* labelnode = getNode(childname);
//			labelnode->accept(*_colorvisitor);
//		}
//	}
//}
//
//void DataManager::fontChangeSlot(osgText::Font* font, int size)
//{
//	_fontvisitor->setFont(font, size);
//	_activatedNode->accept(*_fontvisitor);
//}
//
//void DataManager::sizeChangeSlot(PrimType type, int size)
//{
//	switch (type)
//	{
//	case PRIM_LINE:
//	{
//		osg::LineWidth *linewidth = new osg::LineWidth();
//		linewidth->setWidth(size);
//		_activatedNode->getOrCreateStateSet()->setAttributeAndModes(linewidth, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
//	}
//	break;
//	case PRIM_POINT:
//	{
//		osg::Point *point = new osg::Point();
//		point->setSize(size);
//		_activatedNode->getOrCreateStateSet()->setAttributeAndModes(point, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
//	}
//	break;
//	case PRIM_TEXT:
//	{
//		_fontvisitor->setFont(NULL, size);
//
//		QString currentNodeName = QString::fromStdString(_activatedNode->getName());
//		osg::Group *group = _activatedNode->getParent(0);
//		for (unsigned int i = 0; i < group->getNumChildren(); i++)
//		{
//			QString childname = QString::fromStdString(group->getChild(i)->getName());
//			if (childname.contains(currentNodeName) && childname != currentNodeName)
//			{
//				osg::Node* labelnode = getNode(childname);
//				labelnode->accept(*_fontvisitor);
//			}
//		}
//		_activatedNode->accept(*_fontvisitor);
//	}
//	break;
//	default:
//		break;
//	}
//}
//
//void DataManager::showAttributeTableSlot()
//{
//	QList<QTreeWidgetItem *> itemList = _nodeTree->selectedItems();
//	QTreeWidgetItem *item = itemList.at(0);
//	QString nodeName = item->text(0);
//	auto record = getRecord(nodeName);
//	if (!record)
//		return;
//
//	if (record->_type == SHAPELAYER)
//	{
//		emit showDataAttributes(nodeName);
//	}
//}
//
//void DataManager::changeLayerStyleSlot()
//{
//	// TODO: Fix this
//
//	//using namespace std;
//
//	//QList<QTreeWidgetItem *> itemList = _nodeTree->selectedItems();
//	//QTreeWidgetItem *item = itemList[0];
//	//QString layerName = item->text(0);
//	//osg::Node* layernode = findNodeInNode(layerName.toLocal8Bit().toStdString(), _mapNode[0]);
//
//	//osg::ref_ptr<osgEarth::ModelLayer> layer = dynamic_cast<osgEarth::ModelLayer*>(_mainMap[0]->getLayerByName(layerName.toLocal8Bit().toStdString()));
//
//	//string gemtype;
//	//string iconPath;
//	//float layerHeight = 0;
//	//float layerHeightPre = 0;
//
//	//layernode->getUserValue("gemtype", gemtype);
//	//layernode->getUserValue("layerheight", layerHeightPre);
//
//	//QString lyGemType = QString::fromStdString(gemtype);
//
//	//if (!_featureStyleDlg)
//	//{
//	//	_featureStyleDlg = new FeatureStyleSettingDlg;
//	//}
//
//	//_featureStyleDlg->setLayerStyle(gemtype, 0, layerName, layerHeightPre);
//
//	//if (_featureStyleDlg->exec())
//	//{
//	//	iconPath = _featureStyleDlg->getIconPath().toLocal8Bit().toStdString();
//	//	layerHeight = _featureStyleDlg->getLayerHeight();
//	//}
//	//else
//	//{
//	//	return;
//	//}
//
//	//osg::ref_ptr<osgEarth::ModelLayer> newlayer = layer;
//
//	//if (iconPath != "geometry" && lyGemType == "Point")
//	//	lyGemType = "Icon";
//	//if (iconPath == "geometry" && lyGemType == "Icon")
//	//	lyGemType = "Point";
//
//
//	//newlayer = changeLayerStyle(layerName.toLocal8Bit().toStdString(), lyGemType, LOCAL_FILE, iconPath, layerHeight);
//
//
//	//osgEarth::ModelLayerVector  out_layers;
//	//_mainMap[0]->getLayers(out_layers);
//	//for (int i = 0; i < out_layers.size(); i++)
//	//{
//	//	QString ss = QString::fromStdString(out_layers.at(i)->getName());
//	//	if (ss.contains(layerName) && ss != layerName)
//	//	{
//	//		string labelLayerName = ss.toLocal8Bit().toStdString();
//	//		osg::ref_ptr<osgEarth::ModelLayer> labellayer = dynamic_cast<osgEarth::ModelLayer*>(_mainMap[0]->getLayerByName(labelLayerName));
//	//		_mainMap[0]->removeModelLayer(labellayer);
//	//	}
//	//}
//
//	//if (getDataRecord(layerName))
//	//{
//	//	deleteNode(layerName);
//
//	//	_dataLoader->loadShapeLayer(layerName, LOCAL_FILE, newlayer);
//
//	//	layernode = findNodeInNode(layerName.toLocal8Bit().toStdString(), _mapNode[0]);
//
//	//	layernode->setUserValue("addType", 0);
//
//	//	layernode->setUserValue("gemtype", lyGemType.toLocal8Bit().toStdString());
//
//	//	layernode->setUserValue("layerheight", layerHeight);
//	//}
//
//}

//osgEarth::ModelLayer* DataManager::changeLayerStyle(
//	std::string path, const QString& gemtype, FileType addType, std::string iconPath, float layerHeight)
//{
//	//return _modellyermanager->changeLayerStyle(path, gemtype, addType, iconPath, layerHeight);
//
//	return NULL;
//}

// TODO: Model manipulation should be handled by plugins
//void editModelNodeSlot_move(bool checked);
//void editModelNodeSlot_spin(bool checked);
//void DataManager::editModelNodeSlot_move(bool checked)
//{
//	QList<QTreeWidgetItem *> itemList = _nodeTree->selectedItems();
//	foreach(QTreeWidgetItem *item, itemList)
//	{
//		auto dataType = getDataType(item->text(0));
//
//		if (dataType == MODEL)
//		{
//			QString nodeName = item->text(0);
//
//			if (checked)
//			{
//				item->setData(0, Qt::UserRole, 1);
//				osg::ref_ptr<osg::Node> selectedNode = findNodeInNode(nodeName.toLocal8Bit().toStdString(), _root);
//				selectedNode->setName(nodeName.toLocal8Bit().toStdString());
//				osg::ref_ptr<osg::Node> tempNode4EditModel = addDraggerToScene(selectedNode.get(), false, 0);
//
//				selectedNode->getParent(0)->addChild(tempNode4EditModel.get());
//				selectedNode->getParent(0)->removeChild(selectedNode.get());
//
//				rotateModelAction->setEnabled(false);
//			}
//			else
//			{
//				osg::ref_ptr<osg::Node> selectedNode = findNodeInNode(nodeName.toLocal8Bit().toStdString(), _root);
//				selectedNode->getParent(0)->removeChild(selectedNode.get());
//				item->setData(0, Qt::UserRole, 0);
//
//				rotateModelAction->setEnabled(true);
//			}
//		}
//		else
//		{
//			moveModelAction->setChecked(false);
//			QMessageBox::information(NULL, tr("Warning"), tr("No editable model available!"), QMessageBox::Yes);
//		}
//	}
//}
//
//void DataManager::editModelNodeSlot_spin(bool checked)
//{
//	QList<QTreeWidgetItem *> itemList = _nodeTree->selectedItems();
//	foreach(QTreeWidgetItem *item, itemList)
//	{
//		auto dataType = getDataType(item->text(0));
//
//		if (dataType == MODEL)
//		{
//			QString nodeName = item->text(0);
//
//			if (checked)
//			{
//				item->setData(1, Qt::UserRole, 1);
//				osg::ref_ptr<osg::Node> selectedNode = findNodeInNode(nodeName.toLocal8Bit().toStdString(), _root);
//				selectedNode->setName(nodeName.toLocal8Bit().toStdString());
//				osg::ref_ptr<osg::Node> tempNode4EditModel = addDraggerToScene(selectedNode.get(), false, 1);
//
//				selectedNode->getParent(0)->addChild(tempNode4EditModel.get());
//				selectedNode->getParent(0)->removeChild(selectedNode.get());
//
//				moveModelAction->setEnabled(false);
//			}
//			else
//			{
//				osg::ref_ptr<osg::Node> selectedNode = findNodeInNode("drag" + nodeName.toLocal8Bit().toStdString(), _root);
//				selectedNode->getParent(0)->removeChild(selectedNode.get());
//				item->setData(1, Qt::UserRole, 0);
//
//				moveModelAction->setEnabled(true);
//			}
//		}
//		else
//		{
//			rotateModelAction->setChecked(false);
//			QMessageBox::information(NULL, tr("Warning"), tr("No editable model available!"), QMessageBox::Yes);
//		}
//	}
//}