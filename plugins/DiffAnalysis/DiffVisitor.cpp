#include "DiffVisitor.h"

#include "../../NameSpace.h"

#include <osgUtil/LineSegmentIntersector>
#include <osgDB/ReadFile>

const float  ZERO_LIMIT = 0.1f;

DiffVisitor::DiffVisitor(osg::Vec2Array *boundary, osg::Node *scene, bool loadHighestPLOD):
	NodeVisitor(NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
	_scene(scene),
	_boundary(boundary),
	_loadHighestPLOD(loadHighestPLOD)
{
	_intersector = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, osg::Vec3(), osg::Vec3());
	_iv.setIntersector(_intersector);
	// Compare with window 2 only
	_iv.setTraversalMask(SHOW_IN_WINDOW_1 << 1 | SHOW_IN_NO_WINDOW & INTERSECT_IGNORE);

	_differentPoints = new osg::Vec3Array;
}

DiffVisitor::~DiffVisitor(void)
{
}

void  DiffVisitor::apply(osg::PagedLOD &plod)
{
	// If not required to traverse all lod, then traverse active ones only
	if (!_loadHighestPLOD)
	{
		NodeVisitor::apply(plod);

		return;
	}

	updateMatrix(plod);

	// Before reaching the last lod, check if it's in the interested region
  _affected     = false;
	_deepestLevel = false;

	for (unsigned int i = 0; i < plod.getNumChildren(); i++)
	{
		plod.getChild(i)->accept(*this);
	}

	if (_affected)
	{
		// If this tile intersect with the interested region, traverse its children
		for (unsigned int i = 0; i < plod.getNumFileNames(); i++)
		{
      auto  traversed = _traversed.find(plod.getFileName(i));

			if (traversed == _traversed.end())
			{
				this->setTraversalMode(NodeVisitor::TRAVERSE_ALL_CHILDREN);
        osg::ref_ptr<osg::Node>  nextLevel = osgDB::readNodeFile(plod.getDatabasePath() + '/' + plod.getFileName(i));

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

void  DiffVisitor::apply(osg::Geode &geode)
{
	updateMatrix(geode);

	for (unsigned int i = 0; i < geode.getNumDrawables(); i++)
	{
    osg::Geometry *geom = geode.getDrawable(i)->asGeometry();

		if (geom)
		{
			// If this node intersects with the interested region, calculate difference
			osg::Vec3Array *vertices = (osg::Vec3Array *)geom->getVertexArray();

			if (!_loadHighestPLOD || _affected)
			{
				// Calculate directly for active-only case or affected nodes
				for (int i = 0; i < vertices->size(); i++)
				{
          osg::Vec3  point1 = vertices->at(i) * _localToWorld;

					if (pointInPolygon(point1))
					{
						_intersector->setStart(osg::Vec3d(point1.x(), point1.y(), 10000));
						_intersector->setEnd(osg::Vec3d(point1.x(), point1.y(), -10000));
						_scene->accept(_iv);
            osg::Vec3  point2 = _intersector->getFirstIntersection().getWorldIntersectPoint();

						if (!samePoint(point1, point2))
            {
              _differentPoints->push_back(point1 - _worldOffset);
            }
          }
				}
			}
			else if (_loadHighestPLOD)
			{
				// Mark the lod as affected
				for (int i = 0; i < vertices->size(); i++)
				{
          auto  g = vertices->at(i) * _localToWorld;

          if (pointInPolygon(g))
					{
						_affected = true;

						return;
					}
				}
			}
		}
	}
}

inline bool  DiffVisitor::pointInPolygon(osg::Vec3 &point)
{
  int   i, j, nvert = _boundary->size();
  bool  c = false;

	// Check if the point is inside the boundary (in 2D)
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

inline bool  DiffVisitor::samePoint(const osg::Vec3 &point1, const osg::Vec3 &point2)
{
	return abs(point1.z() - point2.z()) < ZERO_LIMIT;
}
