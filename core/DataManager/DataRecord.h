#pragma once

#include "DataManager_global.h"

#include "../../NameSpace.h"

#include <QTreeWidgetItem>

#include <osg/BoundingSphere>
#include <osg/Node>

namespace osgEarth {
	class GeoExtent;
	class Layer;
}

class DATAMANAGER_EXPORT DataRecord : public QTreeWidgetItem
{
public:
	// Dummy node
	DataRecord(const QString& name, DataRecord* parent);

	// osg node: given bounding box
	DataRecord(const QString& name, osg::Node* node, DataRecord* parent, const osg::BoundingSphere* bs = NULL);

	// OSGEarth layer: given extent
	DataRecord(const QString& name, osgEarth::Layer* layer, DataRecord* parent, const osgEarth::GeoExtent* extent = NULL, int mask = 0xffffffff);

	// OSGEarth layer: given bounding box
	DataRecord(const QString& name, osgEarth::Layer* layer, DataRecord* parent, const osg::BoundingSphere* bs, int mask = 0xffffffff);

    ~DataRecord();

    DataRecord* parent() const;
	DataRecord* child(int index) const;

	// Getters
	osg::Node* node();
	osgEarth::Layer* layer();
	bool isLayer() const;
	
	const osg::BoundingSphere* bounding() const;
	const osgEarth::GeoExtent* extent() const;

	int mask() const;
	void setMask(int mask);

	void setHidden(bool hidden);

	void addChild(DataRecord* child);

	void adjustVisibility();

private:
	union {
		osg::ref_ptr<osg::Node> const _node;
		osg::ref_ptr<osgEarth::Layer> const _layer;
	};

	const osg::BoundingSphere* _bs;
	const osgEarth::GeoExtent* _extent;
	bool const _isLayer;
	int _mask;
};
