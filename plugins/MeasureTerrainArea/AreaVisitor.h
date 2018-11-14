#ifndef AREAVISITOR_H
#define AREAVISITOR_H

#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/Geode>
#include <osg/PagedLOD>

class AreaVisitor : public osg::NodeVisitor
{
public:
	// 要求输入一个相对于(0,0,1)方向逆时针环绕的凸控制边界
	AreaVisitor(osg::Vec2Array* boundary, bool loadDeepestPLOD = false);

	virtual ~AreaVisitor();

	void apply(osg::PagedLOD& plod);

	void apply(osg::Geode& geode);

	float calcTriangle(osg::Vec3& p1, osg::Vec3& p2, osg::Vec3& p3);

	bool pointInPolygon(osg::Vec3& point);

	float calcTriArea(osg::Vec3& p1, osg::Vec3& p2, osg::Vec3& p3)
	{
		return ((p1 - p2) ^ (p1 - p3)).length() / 2;
	}

	float getArea() {return _area;}

	void updateMatrix(osg::Node& node)
	{
		osg::Matrix trans = osg::computeLocalToWorld(node.getParentalNodePaths().front());
		if (!trans.isIdentity())
			_localToWorld = trans;
	}

protected:
	osg::ref_ptr<osg::Vec2Array> _boundary;
	float _area;
	bool _deepestLevel;
	bool _affected;
	std::string _currentFile;
	std::map<std::string, bool> _traversed;
	osg::Matrix _localToWorld;

	bool _loadHighestPLOD;
};

#endif