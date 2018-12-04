#include "AreaVisitor.h"

#include <osgDB/ReadFile>

AreaVisitor::AreaVisitor(osg::Vec2Array *boundary, bool loadHighestPLOD):
  NodeVisitor(NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
	_boundary(boundary),
	_area(.0f),
	_loadHighestPLOD(loadHighestPLOD)
{
}

AreaVisitor::~AreaVisitor()
{
}

void  AreaVisitor::apply(osg::PagedLOD &plod)
{
	// 如果不需要载入最精细层，则只遍历活跃节点
	if (!_loadHighestPLOD)
	{
		NodeVisitor::apply(plod);

		return;
	}

	updateMatrix(plod);

	// 如果没有到最后一层，检查该tile是否在控制边界内
  _affected     = false;
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
      auto  traversed = _traversed.find(plod.getFileName(i));

			if (traversed == _traversed.end())
			{
				this->setTraversalMode(NodeVisitor::TRAVERSE_ALL_CHILDREN);
        osg::ref_ptr<osg::Node>  nextLevel = osgDB::readNodeFile(plod.getDatabasePath() + '/' + plod.getFileName(i));
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

void  AreaVisitor::apply(osg::Geode &geode)
{
	updateMatrix(geode);

	for (unsigned int i = 0; i < geode.getNumDrawables(); i++)
	{
    osg::Geometry *geom = geode.getDrawable(i)->asGeometry();

		if (geom)
		{
      osg::Vec3Array *vertices = (osg::Vec3Array *)geom->getVertexArray();

			if (!_loadHighestPLOD || _affected)
			{
				// 该节点与控制体有交集时，计算交集中的三角形面积
				// 如果_loadHighestPLOD开关为false，则该节点属于活跃节点，可以直接进行计算
        float  area = 0.0f;

        for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); i++)
				{
          osg::PrimitiveSet *prim = geom->getPrimitiveSet(i);

          if ((prim->getMode() == osg::PrimitiveSet::Mode::POINTS) || (prim->getMode() == osg::PrimitiveSet::Mode::LINES)
              || (prim->getMode() == osg::PrimitiveSet::Mode::LINE_STRIP) || (prim->getMode() == osg::PrimitiveSet::Mode::LINE_LOOP))
          {
						continue;
          }

          if (vertices->size() <= prim->index(prim->getNumIndices() - 1))
          {
						continue;
          }

          for (unsigned int i = 0; i + 2 < prim->getNumIndices(); i += 3)
          {
            auto  p1 = vertices->at(prim->index(i)) * _localToWorld;
            auto  p2 = vertices->at(prim->index(i + 1)) * _localToWorld;
            auto  p3 = vertices->at(prim->index(i + 2)) * _localToWorld;
            area += calcTriangle(
              p1,
              p2,
              p3);
					}
				}

				std::cout << _currentFile << ":" << area << std::endl;
				_area += area;
			}
			else if (_loadHighestPLOD)
			{
				// 检查该节点是否与控制体有交集，并标记
				for (int i = 0; i < vertices->size(); i++)
				{
          auto  p = vertices->at(i) * _localToWorld;

          if (pointInPolygon(p))
					{
						_affected = true;

						return;
					}
				}
			}
		}
	}
}

inline float  AreaVisitor::calcTriangle(osg::Vec3 &p1, osg::Vec3 &p2, osg::Vec3 &p3)
{
	if (((int)pointInPolygon(p1) + (int)pointInPolygon(p2) + (int)pointInPolygon(p3)) < 2)
  {
    return .0f;
  }
  else
  {
		return calcTriArea(p1, p2, p3);
  }
}

inline bool  AreaVisitor::pointInPolygon(osg::Vec3 &point)
{
  int   i, j, nvert = _boundary->size();
  bool  c = false;

  for (i = 0, j = nvert - 1; i < nvert; j = i++)
  {
    if (((_boundary->at(i).y() >= point.y()) != (_boundary->at(j).y() >= point.y()))
        && (point.x()
            <= (_boundary->at(j).x() - _boundary->at(i).x()) * (point.y() - _boundary->at(i).y()) / (_boundary->at(j).y() - _boundary->at(i).y())
            + _boundary->at(i).x())
        )
    {
			c = !c;
    }
  }

	return c;
}
