#include "VolumeVisitor.h"

#include <iostream>
#include <limits>
using namespace std;

#include <osg/Geometry>
#include <osgDB/ReadFile>

VolumeVisitor::VolumeVisitor(osg::Vec3Array* boundary3D, osg::Plane& refPlane, bool loadHighestPLOD /*= false*/)
	: NodeVisitor( NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
	_volume(.0f),
	_boundary3D(boundary3D),
	_done(false),
	_loadHighestPLOD(loadHighestPLOD),
	_refPlane(refPlane)
{
	// 保存一个2D的控制轮廓
	_boundary2D = new osg::Vec2Array;
	for(int i = 0; i < boundary3D->size(); i++)
		_boundary2D->push_back(osg::Vec2(boundary3D->at(i).x(), boundary3D->at(i).y()));

	// 获得各计算控制点
	int numCtrlPoints = boundary3D->size();
	osg::Vec4 plane = _refPlane.asVec4();
	_minHeight = numeric_limits<float>::max();
	for (int i = 0; i < numCtrlPoints; i++)
	{
		osg::Vec2 point2D = _boundary2D->at(i);
		float height = (-plane.w() - plane.x() * point2D.x() - plane.y() * point2D.y()) / plane.z();
		// 1. 控制边界与基平面的交点
		_p1[i] = osg::Vec3(point2D, height);
		// 2. 控制边界与场景的交点
		_p2[i] = boundary3D->at(i);
		// 找到底平面的高度
		if (height < _minHeight)
			_minHeight = height;
	}

	for (int i = 0; i < numCtrlPoints; i++)
	{
		// 3. 控制边界与底平面的交点
		_p0[i] = osg::Vec3(_boundary2D->at(i), _minHeight);
	}

	// 估算控制体的重心
	for (int i = 0; i < boundary3D->size(); i++)
	{
		_apex += boundary3D->at(i);
	}
	_apex /= boundary3D->size();
	_apex.z() = _minHeight + (_apex.z() - _minHeight) / 2;
}

VolumeVisitor::~VolumeVisitor()
{
}

inline float VolumeVisitor::calcTriangleToApex(const osg::Vec3& p1, const osg::Vec3& p2, const osg::Vec3& p3)
{
	if ( ((int)pointInPolygon(p1) + (int)pointInPolygon(p2) + (int)pointInPolygon(p3)) < 2)
		return 0.0f;
	else
		return calcPyramidVol(p1, p2, p3, _apex);
}

float VolumeVisitor::getVolume()
{
	if (!_done)
	{
		// 若计算结果为负，说明控制点重心在包围体内，取其绝对值
		cout << "surface vol:" << _volume << endl;
		_volume = abs(_volume);
		
		// 计算侧面和底面到重心的体积
		_volume += calcSideBottomVol(_boundary3D, _apex);

		// 减去基平面到底平面的体积
		_volume -= calcVolumeUnderBase(_boundary3D);

		_done = true;
	}
	return _volume;
}

inline float VolumeVisitor::calcSideBottomVol(const osg::Vec3Array* boundary3D, const osg::Vec3& apex)
{
	int numCtrlPoints = boundary3D->size();

	// 计算侧面到重心的体积
	float sideVol = 0.0f;
	for (int i = 0; i < numCtrlPoints; i++)
	{
		sideVol += calcPyramidVol(_p2[i], _p2[(i+1) % numCtrlPoints], _p0[i], apex);
		sideVol += calcPyramidVol(_p2[(i+1) % numCtrlPoints], _p0[(i+1) % numCtrlPoints], _p0[i], apex);
	}

	// 计算底面到重心的体积
	float bottomVol = 0.0f;
	for (int i = 0; i < numCtrlPoints - 2; i++)
	{
		bottomVol += calcPyramidVol(_p0[0], _p0[i + 1], _p0[i + 2], apex);
	}

	cout << "side vol:" << sideVol << endl;
	cout << "bottom vol:" << bottomVol << endl;

	return sideVol + bottomVol;
}

float VolumeVisitor::calcVolumeUnderBase(const osg::Vec3Array* boundary3D)
{
	float volume = .0f;
	// 斜截棱柱的体积计算：将棱柱分为多个斜截三棱柱来计算体积
	for (int i = 0; i < boundary3D->size() - 2; i++)
	{
		float area = abs((_p0[0] - _p0[i + 1]) * (_p0[i + 2] - _p0[i + 1]));
		volume += area * (_p1[0].z() - _p0[0].z() + _p1[i + 1].z() - _p0[i + 1].z() + _p1[i + 2].z() - _p0[i + 2].z()) / 3;
	}
	cout << "cut off vol: " << volume << endl;
	return volume;
}

void VolumeVisitor::apply(osg::Geode& geode)
{
	updateMatrix(geode);
	for (unsigned int i = 0; i < geode.getNumDrawables(); i++)
	{
		osg::Geometry* geom = geode.getDrawable(i)->asGeometry();
		if (geom)
		{
			osg::Vec3Array* vertices = (osg::Vec3Array*)geom->getVertexArray();
			if (!_loadHighestPLOD || _affected)
			{
				// 该节点与控制体有交集时，计算交集中的三角形面积
				// 如果_loadHighestPLOD开关为false，则该节点属于活跃节点，可以直接进行计算
				float volume = 0.0f;
				for(unsigned int i = 0; i < geom->getNumPrimitiveSets(); i++)
				{
					osg::PrimitiveSet* prim = geom->getPrimitiveSet(i);

					if (prim->getMode() == osg::PrimitiveSet::Mode::POINTS || prim->getMode() == osg::PrimitiveSet::Mode::LINES ||
						prim->getMode() == osg::PrimitiveSet::Mode::LINE_STRIP || prim->getMode() == osg::PrimitiveSet::Mode::LINE_LOOP)
						continue;
					if (vertices->size() <= prim->index(prim->getNumIndices() - 1))
						continue;
					for (unsigned int i = 0; i+2 < prim->getNumIndices(); i+=3)
					{
						volume += calcTriangleToApex(
							vertices->at(prim->index(i)) * _localToWorld,
							vertices->at(prim->index(i+1)) * _localToWorld,
							vertices->at(prim->index(i+2)) * _localToWorld);
					}
				}
				std::cout << _currentFile << ":" << volume << std::endl;
				_volume += volume;
			}
			else if (_loadHighestPLOD)
			{
				// 检查该节点是否与控制体有交集
				for (int i = 0; i < vertices->size(); i++)
				{
					if (pointInPolygon(vertices->at(i)* _localToWorld))
					{
						_affected = true;
						return;
					}
				}
			}
		}
	}
}

void VolumeVisitor::apply(osg::PagedLOD& plod)
{
	// 如果不需要载入最精细层，则用普通方式遍历
	if (!_loadHighestPLOD)
	{
		NodeVisitor::apply(plod);
		return;
	}
	updateMatrix(plod);

	// 如果没有到最后一层，检查该tile是否在控制边界内
	_affected = false;
	_deepestLevel = false;
	for (unsigned int i = 0; i < plod.getNumChildren(); i++)
	{
		plod.getChild(i)->accept(*this);
	}

	if (_affected)
	{
		// 继续遍历下一层
		for (unsigned int i = 0; i < plod.getNumFileNames(); i++)
		{
			auto traversed = _traversed.find(plod.getFileName(i));
			if (traversed == _traversed.end())
			{
				this->setTraversalMode(NodeVisitor::TRAVERSE_ALL_CHILDREN);
				osg::ref_ptr<osg::Node> nextLevel = osgDB::readNodeFile(plod.getDatabasePath() + '/' + plod.getFileName(i));
				_currentFile = plod.getFileName(i);
				if (nextLevel)
				{
					nextLevel->accept(*this);
					_traversed[plod.getFileName(i)] = true;
				}
			}
		}								
	}
	this->setTraversalMode(NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
}

inline bool VolumeVisitor::pointInPolygon(const osg::Vec3& point)
{
	int i, j, nvert = _boundary2D->size();
	bool c = false;

	for(i = 0, j = nvert - 1; i < nvert; j = i++) {
		if( ( (_boundary2D->at(i).y() >= point.y() ) != (_boundary2D->at(j).y() >= point.y()) ) &&
			(point.x() <= (_boundary2D->at(j).x()- _boundary2D->at(i).x()) * (point.y() - _boundary2D->at(i).y()) / (_boundary2D->at(j).y()- _boundary2D->at(i).y()) + _boundary2D->at(i).x())
			)
			c = !c;
	}
	return c;
}