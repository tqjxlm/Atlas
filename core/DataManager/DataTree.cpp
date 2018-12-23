#include "DataTree.h"

#include <QMessageBox>
#include <QFileDialog>

#include <osg/Node>
#include <osg/Group>
#include <osgSim/OverlayNode>
#include <osgDB/WriteFile>

#include <osgEarth/Map>
#include <osgEarth/TerrainLayer>
#include <osgEarth/ImageLayer>
#include <osgEarth/ModelLayer>
#include <osgEarth/ElevationLayer>

#include <SettingsManager/SettingsManager.h>

DataTree::DataTree(SettingsManager *settings, QWidget *parent):
	QTreeWidget(parent),
	_settingsManager(settings)
{
	// Init tree
	_rootTreeItem = new DataRecord(tr("Scene"), NULL);
	addTopLevelItem(_rootTreeItem);
	_rootTreeItem->setExpanded(true);
}

DataTree::~DataTree()
{
}

void  DataTree::clear()
{
	for (unsigned i = 0; i < _rootTreeItem->childCount(); i++)
	{
		removeRecord(_rootTreeItem->child(i));
	}
}

void  DataTree::addRecord(osg::Node *node, const QString &name, const QString &parentName, bool hidden)
{
	QString              nodeName = resolveName(name);
	osg::BoundingSphere  bs       = node->getBound();

	if (!bs.valid())
	{
		node->computeBound();
	}

	node->setName(nodeName.toLocal8Bit().toStdString());

	DataRecord *parent    = getParent(parentName);
	DataRecord *newRecord = new DataRecord(nodeName, node, parent);
	newRecord->setCheckState(0, (node->getNodeMask() & SHOW_IN_ALL_WINDOW) == 0 ? Qt::Unchecked : Qt::Checked);
	newRecord->setHidden(hidden);
	_dataRecords.insert(nodeName, newRecord);

	parent->addChild(newRecord);
}

void  DataTree::addRecord(osgEarth::Layer *layer, const QString &name, const QString &parentName, osgEarth::GeoExtent *extent, bool hidden)
{
	QString  nodeName = resolveName(name);

	layer->setName(nodeName.toLocal8Bit().toStdString());

	DataRecord *parent    = getParent(parentName);
	DataRecord *newRecord = new DataRecord(nodeName, layer, parent, extent);
	newRecord->setCheckState(0, layer->getEnabled() ? Qt::Checked : Qt::Unchecked);
	newRecord->setHidden(hidden);
	_dataRecords.insert(nodeName, newRecord);

	parent->addChild(newRecord);
}

void  DataTree::switchDataSlot(QTreeWidgetItem *item, int column)
{
	auto  nodeName = item->text(column);

	switchRecord(nodeName, item->checkState(0) == Qt::Checked);
}

void  DataTree::switchAllSlot(int enabled)
{
	QList<QTreeWidgetItem *>  itemList = selectedItems();
	DataRecord               *item     = dynamic_cast<DataRecord *>(itemList[0]);

	switchAll(item, enabled);
}

void  DataTree::switchAll(DataRecord *item, bool isVisible)
{
	int  childcount = item->childCount();

	for (int i = 0; i < childcount; i++)
	{
		DataRecord *childitem = item->child(i);

		if (childitem->childCount() > 0)
		{
			switchAll(childitem, isVisible);
		}
		else
		{
			switchRecord(childitem->text(0), isVisible);
		}
	}
}

QString  DataTree::resolveName(const QString &expectedName)
{
	// Name format: ExpectedName + _ + Index
	QString  newName = expectedName;
	int      pos     = newName.lastIndexOf('_');
	int      index   = pos == -1 ? 0 : newName.right(pos).toInt();

	// Increase Index until it does not collide
	for (unsigned i = index + 1; i < 0xffffffff; i++)
	{
		newName = expectedName + QString("_%1").arg(i);

		if (getRecord(newName) == NULL)
		{
			return newName;
		}
	}

	throw std::runtime_error("Run out of node names because of duplication.");

	return "";
}

void  DataTree::removeDataRendering(DataRecord *record)
{
	if (record->isLayer())
	{
		for (int i = 0; i < MAX_SUBVIEW; i++)
		{
			osgEarth::Layer *existed = _mainMap[i]->getLayerByName(record->layer()->getName());

			if (existed)
			{
				_mainMap[i]->removeLayer(existed);
			}
		}
	}
	else
	{
		osg::Node *node = record->node();

		if (node)
		{
			if ((node->getNumParents() == 0) && node->asGroup())
			{
				// For a pseudo node, parse all of its direct children
				for (unsigned i = 0; i < node->asGroup()->getNumChildren(); i++)
				{
					removeNode(node->asGroup()->getChild(i));
				}
			}
			else
			{
				// For a real rendered node, simply handle
				removeNode(node);
			}
		}
	}
}

void  DataTree::removeNode(osg::Node *node)
{
	// Remove the node from its parent
	for (auto parent : node->getParents())
	{
		parent->removeChild(node);

		// Update overlay
		if (parent == _overlayNode->getOverlaySubgraph())
		{
			_overlayNode->dirtyOverlayTexture();
		}
	}
}

void  DataTree::switchRecord(const QString &nodeName, bool checked)
{
	auto  record = getRecord(nodeName);

	if (record == NULL)
	{
		return;
	}

	if (record->isLayer())
	{
		if (osgEarth::ModelLayer *layer = dynamic_cast<osgEarth::ModelLayer *>(record->layer()))
		{
			for (int i = 0; i < MAX_SUBVIEW; i++)
			{
				layer->setVisible(checked);
			}
		}
		else if (osgEarth::TerrainLayer *layer = dynamic_cast<osgEarth::TerrainLayer *>(record->layer()))
		{
			for (int i = 0; i < MAX_SUBVIEW; i++)
			{
				layer->setVisible(checked);
			}
		}
		else
		{
			for (int i = 0; i < MAX_SUBVIEW; i++)
			{
				record->layer()->setEnabled(checked);
			}
		}
	}
	else
	{
		osg::Node *node = record->node();

		if (node)
		{
			if ((node->getNumParents() == 0) && node->asGroup())
			{
				// For a pseudo node, parse all of its direct children
				for (unsigned i = 0; i < node->asGroup()->getNumChildren(); i++)
				{
					switchNode(node->asGroup()->getChild(i), checked);
				}
			}
			else
			{
				// For a real rendered node, simply handle
				switchNode(node, checked);
			}
		}
	}

	int  mask = record->mask();
	record->setMask(checked ? (mask | SHOW_IN_ALL_WINDOW) : (mask & SHOW_IN_NO_WINDOW));

	if ((record->checkState(0) == Qt::Checked) && (checked == false))
	{
		record->setCheckState(0, Qt::Unchecked);
	}
}

void  DataTree::switchNode(osg::Node *node, bool checked)
{
	// Show in all view
	int  mask = node->getNodeMask();

	node->setNodeMask(checked ? (mask | SHOW_IN_ALL_WINDOW) : (mask & SHOW_IN_NO_WINDOW));

	for (auto parent : node->getParents())
	{
		if (parent == _overlayNode->getOverlaySubgraph())
		{
			_overlayNode->dirtyOverlayTexture();
		}
	}
}

DataRecord * DataTree::getParent(const QString &parentName)
{
	DataRecord *parent = getRecord(parentName);

	if (!parent)
	{
		parent = new DataRecord(parentName, _rootTreeItem);
		_rootTreeItem->addChild(parent);
		_dataRecords.insert(parentName, parent);
		parent->setExpanded(true);
	}

	return parent;
}

void  DataTree::saveNodeSlot()
{
	QList<QTreeWidgetItem *>  itemList = selectedItems();

	if (itemList.size() == 1)
	{
		if (saveNode(itemList[0]->text(0)) == false)
		{
			QMessageBox::critical(0, tr("Error"), tr("Failed to save layer"));
		}
		else
		{
			QMessageBox::information(0, tr("Result"), tr("Successfully saved"));
		}
	}
	else if (itemList.size() > 1)
	{
		QStringList  nameList;

		for (auto item : itemList)
		{
			nameList.append(item->text(0));
		}

		if (saveNodes(nameList) == false)
		{
			QMessageBox::critical(0, tr("Error"), tr("Failed to save layer"));
		}
		else
		{
			QMessageBox::information(0, tr("Result"), tr("Successfully saved"));
		}
	}
}

void  DataTree::removeRecord(const QString &name)
{
	auto  recordItr = _dataRecords.find(name);

	if (recordItr == _dataRecords.end())
	{
		return;
	}

	DataRecord *record = *recordItr;

	if (record == _rootTreeItem)
	{
		return;
	}

	removeRecord(record);

	// Remove from parent
	DataRecord *parent = record->parent();
	parent->removeChild(record);

	if (parent->childCount() == 0)
	{
		removeRecord(parent->text(0));
	}
}

void  DataTree::removeRecord(DataRecord *record)
{
	// Remove children
	for (std::size_t i = 0; i < record->childCount(); i++)
	{
		removeRecord(record->child(i));
	}

	// Remove this
	removeDataRendering(record);
	_dataRecords.erase(_dataRecords.find(record->text(0)));
}

void  DataTree::deleteNodeSlot()
{
	QList<QTreeWidgetItem *>  itemList = selectedItems();

	for (auto item : itemList)
	{
		removeRecord(item->text(0));
	}
}

bool  DataTree::saveNode(const QString &nodeName)
{
	osg::ref_ptr<osg::Node>  node = getNode(nodeName);

	if (!node.valid())
	{
		return false;
	}

	QString  path = QFileDialog::getSaveFileName(0, tr("Choose save location"), " ",
	                                             tr("OSG plain text (*.osg);;OSG binary (*.osgb);;Allfile(*.*)"));

	return osgDB::writeNodeFile(*node, path.toStdString());
}

bool  DataTree::saveNodes(const QStringList &nodeNames)
{
	osg::ref_ptr<osg::Group>  group = new osg::Group();

	for (auto nodeName : nodeNames)
	{
		osg::ref_ptr<osg::Node>  node = getNode(nodeName);

		if (node.valid())
		{
			group->addChild(node);
		}
	}

	if (group->getNumChildren() == 0)
	{
		return false;
	}

	QString  path = QFileDialog::getSaveFileName(0, tr("Choose save location"), " ",
	                                             tr("OSG plain text (*.osg);;OSG binary (*.osgb);;Allfile(*.*)"));

	return osgDB::writeNodeFile(*group, path.toStdString());
}

void  DataTree::setWindowMask(const QString &name, int mask)
{
	auto  record = getRecord(name);

	if (!record)
	{
		return;
	}

	// Only care about the top 4 bits
	int  windowMask = mask & SHOW_IN_ALL_WINDOW;

	if (record->isLayer())
	{
		// Since osgEarth layer don't support mask, we have to move them manually
		osgEarth::Layer *layer   = record->layer();
		std::string      stdName = layer->getName();

		if (!layer)
		{
			return;
		}

		// layer->getOrCreateNode()->setNodeMask(mask);
		for (int i = 0; i < MAX_SUBVIEW; i++)
		{
			if (_mainMap[i] == NULL)
			{
				continue;
			}

			osgEarth::Layer *existed = _mainMap[i]->getLayerByName(stdName);

			if (existed && !(windowMask & (SHOW_IN_WINDOW_1 << i)))
			{
				_mainMap[i]->removeLayer(existed);
			}
			else if (!existed && windowMask & (SHOW_IN_WINDOW_1 << i))
			{
				_mainMap[i]->addLayer(layer);
			}
		}
	}

	record->setMask((record->mask() & SHOW_IN_NO_WINDOW) | windowMask);
}

DataRecord * DataTree::getRecord(const QString &name)
{
	auto  record = _dataRecords.find(name);

	if (record != _dataRecords.end())
	{
		return *record;
	}
	else
	{
		return NULL;
	}
}

osg::Node * DataTree::getNode(const QString &name)
{
	DataRecord *record = getRecord(name);

	if (record)
	{
		return record->node();
	}
	else
	{
		return NULL;
	}
}

osgEarth::Layer * DataTree::getLayer(const QString &name)
{
	DataRecord *record = getRecord(name);

	if (record)
	{
		return record->layer();
	}
	else
	{
		return NULL;
	}
}
