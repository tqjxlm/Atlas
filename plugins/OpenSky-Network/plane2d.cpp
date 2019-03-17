#include "plane2d.h"

Plane2D::Plane2D():
  osgEarth::GeoTransform()
{
}

Data  Plane2D::operator[](const std::string &key)
{
  return _properties[key];
}

const Data  Plane2D::operator[](const std::string &key) const
{
  return _properties[key];
}

void  Plane2D::addAutoTransform(osg::AutoTransform *node)
{
  _child = node;
  addChild(node);
}

void  Plane2D::setRotation(double deg)
{
  osg::Quat  q = osg::Quat(osg::DegreesToRadians(deg), osg::Vec3(0, 1, 0))
                 * osg::Quat(osg::DegreesToRadians(deg), osg::Vec3(1, 0, 0))
                 * osg::Quat(osg::DegreesToRadians(deg), osg::Vec3(0, 0, -1));

  _child->setRotation(q);
}
