#ifndef VOLUMEVISITOR_H
#define VOLUMEVISITOR_H

#include <vector>
#include <map>
#include <string>

#include <osg/NodeVisitor>
#include <osg/Plane>
#include <osg/Matrix>
#include <osg/Geode>
#include <osg/PagedLOD>

class VolumeVisitor : public osg::NodeVisitor
{
public:
	// 要求输入一个相对于(0,0,1)方向逆时针环绕的凸控制边界
	VolumeVisitor(osg::Vec3Array* boundary3D, osg::Plane& refPlane, bool loadHighestPLOD = false);

	virtual ~VolumeVisitor();

	void apply(osg::Geode& geode);

	void apply(osg::PagedLOD& plod);

	bool pointInPolygon(const osg::Vec3& point);

	float calcTriangleToApex(const osg::Vec3& p1, const osg::Vec3& p2, const osg::Vec3& p3);

	float getVolume();

	float calcPyramidVol(const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3, const osg::Vec3& apex)
	{
		return ((v3 -  v2) ^ (v1 - v2)) * (apex - v2) / 6;
	}

	float calcSideBottomVol(const osg::Vec3Array* boundary3D, const osg::Vec3& apex);

	float calcVolumeUnderBase(const osg::Vec3Array* boundary3D);

	void updateMatrix(osg::Node& node)
	{
		osg::Matrix trans = osg::computeLocalToWorld(node.getParentalNodePaths().front());
		if (!trans.isIdentity())
			_localToWorld = trans;
	}

protected:
	osg::ref_ptr<osg::Vec2Array> _boundary2D;
	osg::ref_ptr<osg::Vec3Array> _boundary3D;
	float _volume;
	osg::Vec3 _apex;
	bool _done;

	float _minHeight;
	osg::Plane _refPlane;
	osg::Vec3 _p0[3];
	osg::Vec3 _p1[3];
	osg::Vec3 _p2[3];

	bool _deepestLevel;
	bool _affected;
	std::string _currentFile;
	std::map<std::string, bool> _traversed;
	osg::Matrix _localToWorld;

	bool _loadHighestPLOD;
};

#endif
