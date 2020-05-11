#include "DataRecord.h"

#include <osg/Group>

#include <osgEarth/TerrainLayer>
#include <osgEarth/GeoData>

DataRecord::DataRecord(const QString& name, DataRecord *parent):
  QTreeWidgetItem(parent, QStringList(name)),
  _bs(NULL),
  _extent(NULL),
  _node(NULL),
  _isLayer(false)
{
	setToolTip(0, name);
}

DataRecord::DataRecord(const QString& name, osg::Node *node, DataRecord *parent, const osg::BoundingSphere *bs):
  QTreeWidgetItem(parent, QStringList(name)),
  _node(node),
  _bs(bs ? bs : &node->getBound()),
  _extent(NULL),
  _isLayer(false)
{
	setToolTip(0, name);
}

DataRecord::DataRecord(const QString& name, osgEarth::Layer* layer, DataRecord* parent, const osgEarth::GeoExtent* extent, int mask /*= 0xffffffff*/)
	: QTreeWidgetItem(parent, QStringList(name))
	, _layer(layer)
	, _bs(NULL)
	, _mask(mask)
	, _isLayer(true)
{
  if (extent)
    _extent = extent;
  else if (layer->getExtent().isValid())
    _extent = &layer->getExtent();
  else
  {
    auto node = _layer->getNode();
    if (node)
    {
      _extent = NULL;
      _bs = &node->getBound();
    }
    else
    {
      _bs = NULL;
    }
  }
	setToolTip(0, name);
}

DataRecord::DataRecord(const QString& name, osgEarth::Layer* layer, DataRecord* parent, const osg::BoundingSphere* bs, int mask /*= 0xffffffff*/)
	: QTreeWidgetItem(parent, QStringList(name))
	, _layer(layer)
	, _bs(bs)
	, _extent(NULL)
	, _mask(mask)
	, _isLayer(true)
{
	setToolTip(0, name);
}

DataRecord::~DataRecord()
{
}

DataRecord * DataRecord::parent() const
{
  return dynamic_cast<DataRecord *>(QTreeWidgetItem::parent());
}

DataRecord * DataRecord::child(int index) const
{
  return dynamic_cast<DataRecord *>(QTreeWidgetItem::child(index));
}

osg::Node * DataRecord::node()
{
  return isLayer() ? NULL : _node;
}

osgEarth::Layer* DataRecord::layer()
{
  return isLayer() ? _layer : NULL;
}

bool  DataRecord::isLayer() const
{
  return _isLayer;
}

const osg::BoundingSphere * DataRecord::bounding() const
{
  return _bs;
}

const osgEarth::GeoExtent * DataRecord::extent() const
{
  return _extent;
}

int  DataRecord::mask() const
{
  return isLayer() ? _mask : _node ? _node->getNodeMask() : _mask;
}

void  DataRecord::setMask(int mask)
{
  if (isLayer())
  {
    _mask = mask;
  }
  else if (_node)
  {
    _node->setNodeMask(mask);
  }
  else
  {
    _mask = mask;
  }

// isLayer() ? _mask = mask : _node ? _node->setNodeMask(mask) : _mask = mask;
}

void  DataRecord::setHidden(bool hidden)
{
	QTreeWidgetItem::setHidden(hidden);

  DataRecord *parent = this->parent();

	if (parent)
  {
    parent->adjustVisibility();
  }
}

void  DataRecord::addChild(DataRecord *child)
{
	QTreeWidgetItem::addChild(child);

	if (!child->isHidden())
  {
    setHidden(false);
  }
}

void  DataRecord::adjustVisibility()
{
  bool  parentHidden = true;

	for (unsigned i = 0; i < childCount(); i++)
	{
		if (!child(i)->isHidden())
		{
			parentHidden = false;
			break;
		}
	}

	setHidden(parentHidden);
}
