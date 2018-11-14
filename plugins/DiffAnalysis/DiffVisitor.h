#pragma once

#include <osg/NodeVisitor>
#include <osg/PagedLOD>
#include <osg/Geode>
#include <osg/Matrix>
#include <osgUtil/IntersectionVisitor>

namespace osgUtil {
	class LineSegmentIntersector;
}

class DiffVisitor :
	public osg::NodeVisitor
{
public:
	DiffVisitor(osg::Vec2Array* boundary, osg::Node* scene, bool loadDeepestPLOD = false);
	virtual ~DiffVisitor();

	void apply(osg::PagedLOD& plod);

	void apply(osg::Geode& geode);

	bool pointInPolygon(osg::Vec3& point);
	
	bool samePoint(const osg::Vec3& point1, const osg::Vec3& point2);

	void updateMatrix(osg::Node& node)
	{
		osg::Matrix trans = osg::computeLocalToWorld(node.getParentalNodePaths().front());
		if (!trans.isIdentity())
			_localToWorld = trans;
	}

	osg::Vec3Array* getDifferentPoints()
	{
		return _differentPoints;
	}

	void setWorldOffset(osg::Vec3& offset)
	{
		_worldOffset = offset;
	}

protected:
	osg::ref_ptr<osg::Vec2Array> _boundary;
	bool _deepestLevel;
	bool _affected;
	bool _loadHighestPLOD;
	std::map<std::string, bool> _traversed;

	osg::Vec3 _worldOffset;
	osg::Matrix _localToWorld;
	osg::ref_ptr<osg::Vec3Array> _differentPoints;

	osg::ref_ptr<osgUtil::LineSegmentIntersector> _intersector;
	osgUtil::IntersectionVisitor _iv;
	osg::Node* _scene;	
};

