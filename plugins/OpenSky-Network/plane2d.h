#ifndef PLANE2D_H
#define PLANE2D_H

#include <osgEarth/GeoTransform>
#include <osg/AutoTransform>
#include "dataptree.h"

class Plane2D: public osgEarth::GeoTransform
{
public:
  Plane2D();

  Data        operator[](const std::string &key);

  const Data  operator[](const std::string &key) const;

  void        addAutoTransform(osg::AutoTransform *node);

  void        setRotation(double deg);

private:
  mutable DataPTree                 _properties;
  osg::ref_ptr<osg::AutoTransform>  _child;
};

#endif // PLANE2D_H
