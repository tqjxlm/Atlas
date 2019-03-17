#include "openskyfetcher.h"
#include <iostream>
#include <osgEarth/MapNode>
#include <osgEarth/ThreadingUtils>
#include <osgEarth/Metrics>
#include <osgEarthAnnotation/PlaceNode>
#include <osgEarthAnnotation/AnnotationUtils>
#include <osg/Geode>
#include <osg/Depth>
#include "dataptree.h"
#include <QNetworkReply>
#include <osgEarth/VirtualProgram>

using namespace osgEarth;

namespace
{
const char *iconVS = "#version " GLSL_VERSION_STR "\n"
                                                  "out vec2 oe_PlaceNode_texcoord; \n"
                                                  "void oe_PlaceNode_icon_VS(inout vec4 vertex) \n"
                                                  "{ \n"
                                                  "    oe_PlaceNode_texcoord = gl_MultiTexCoord0.st; \n"
                                                  "} \n";
const char *iconFS = "#version " GLSL_VERSION_STR "\n"
                                                  "in vec2 oe_PlaceNode_texcoord; \n"
                                                  "uniform sampler2D oe_PlaceNode_tex; \n"
                                                  "void oe_PlaceNode_icon_FS(inout vec4 color) \n"
                                                  "{ \n"
                                                  "    color = texture(oe_PlaceNode_tex, oe_PlaceNode_texcoord); \n"
                                                  "} \n";
}

OpenSkyFetcher::OpenSkyFetcher(osg::ref_ptr<MapNode> mapnode, osgViewer::View *view):
  mMapNode(mapnode),
  mMainViewer(view)
{
  setTerminationEnabled(true);
// moveToThread(this);
  mIsUnderFetching = false;


  osg::ref_ptr<osg::StateSet>  _imageStateSet;
  _imageStateSet = new osg::StateSet();
  VirtualProgram *vp = VirtualProgram::getOrCreate(_imageStateSet.get());
  vp->setName("PlaceNode::imageStateSet");
  vp->setFunction("oe_PlaceNode_icon_VS", iconVS, ShaderComp::LOCATION_VERTEX_MODEL);
  vp->setFunction("oe_PlaceNode_icon_FS", iconFS, ShaderComp::LOCATION_FRAGMENT_COLORING);
  _imageStateSet->addUniform(new osg::Uniform("oe_PlaceNode_tex", 0));
  _imageStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  _imageStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

  osg::Image *pImage = osgDB::readImageFile("resources/OpenSkyNetwork/opensky.png");

  if (!pImage)
  {
    std::cout << "Error: Couldn't find texture!" << std::endl;
  }


  osg::Geometry *imageGeom = osgEarth::Annotation::AnnotationUtils::createImageGeometry(
    pImage,                        // image
    osg::Vec2s(0, 0),             // offset
    0,                            // tex image unit
    0,
    1);

  imageGeom->getOrCreateStateSet()->merge(*_imageStateSet.get());


  mModel2D = new osg::AutoTransform;
  mModel2D->setAutoScaleToScreen(true);
  mModel2D->addChild(imageGeom);
  mModel2D->setMaximumScale(1000.0);
  mModel2D->setMinimumScale(50.0);
  mModel2D->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);

  _group = new osg::Group;
  _group->setCullingActive(false);

  mMapNode->addChild(_group);
}

void  OpenSkyFetcher::stopTimer()
{
  mFetchTimer->stop();
}

void  OpenSkyFetcher::httpFinished(QNetworkReply *reply)
{
  std::string  buf = reply->readAll().toStdString();

  if (buf.empty())
  {
    return;
  }

  DataPTree  mBuffer;
  try
  {
    mBuffer.from_json(buf);
  }
  catch (...)
  {
    return;
  }

  int                               size   = mBuffer["states"].count();
  const osgEarth::SpatialReference *geoRef = mMapNode->getMapSRS();
  const SpatialReference           *wgs84  = SpatialReference::get("wgs84");

  for (int i = 0; i < size; ++i)
  {
    double  x, y, z, vel;
    x   = mBuffer["states"].at(i).at(5).defaults_to<double>(0);
    y   = mBuffer["states"].at(i).at(6).defaults_to<double>(0);
    z   = mBuffer["states"].at(i).at(7).defaults_to<double>(0);
    vel = mBuffer["states"].at(i).at(9).defaults_to<double>(0);

    if ((x == 0) || (y == 0) || (z == 0))
    {
      continue;
    }

    std::string  ico24 = mBuffer["states"].at(i).at(0).defaults_to<std::string>("");
    int          id    = std::stoi(ico24, nullptr, 16);

    if (mModels.contains(id))
    {
      mModels[id]->ref();
      mModels[id]->setPosition(osgEarth::GeoPoint(geoRef, x, y, z));
      mModels[id]->unref();
    }
    else
    {
      osg::ref_ptr<Plane2D>  plane = new Plane2D;

      (*plane)["id"]             = id;
      (*plane)["ico24"]          = ico24;
      (*plane)["CallSign"]       = mBuffer["states"].at(i).at(1).defaults_to<std::string>("");
      (*plane)["Velocity"]       = vel;
      (*plane)["OnGround"]       = mBuffer["states"].at(i).at(8).defaults_to<bool>(false);
      (*plane)["Vertical_Speed"] = mBuffer["states"].at(i).at(11).defaults_to<double>(0.0);
      (*plane)["SQUAWK"]         = mBuffer["states"].at(i).at(14).defaults_to<std::string>("");
      (*plane)["Position_Time"]  = mBuffer["states"].at(i).at(3).defaults_to<long long>(0);
      (*plane)["velocity"]       = vel;

      switch (mBuffer["states"].at(i).at(16).defaults_to<int>(0))
      {
      case 0:
        (*plane)["Position_Source"] = "ADS-B";
        break;
      case 1:
        (*plane)["Position_Source"] = "ASTERIX";
        break;
      case 2:
        (*plane)["Position_Source"] = "MLAT";
        break;
      default:
        (*plane)["Position_Source"] = "Unknown";
      }

      (*plane)["Source"] = "OpenSky";

      plane->addAutoTransform(mModel2D.get());
      plane->setPosition(osgEarth::GeoPoint(wgs84, x, y, z));
      plane->setRotation(45.0);

      _group->ref();
      _group->addChild(plane);
      _group->unref();
      mModels[id] = plane;
    }
  }

  mIsUnderFetching = false;
}

void  OpenSkyFetcher::fetchdata()
{
  if (mIsUnderFetching)
  {
    return;
  }

  mIsUnderFetching = true;
  mNetMgr->get(*mNetRequest);
}

void  OpenSkyFetcher::run()
{
  mNetRequest = new QNetworkRequest();
  mNetMgr     = new QNetworkAccessManager();
  connect(mNetMgr, &QNetworkAccessManager::finished, this, &OpenSkyFetcher::httpFinished, Qt::QueuedConnection);
  mNetRequest->setUrl(QUrl(QString("https://opensky-network.org/api/states/all")));

  mFetchTimer = new QTimer();
  connect(mFetchTimer, &QTimer::timeout, this, &OpenSkyFetcher::fetchdata, Qt::QueuedConnection);
  mFetchTimer->start(10000);

// exec();

// mFetchTimer->stop();
// disconnect(mNetMgr, &QNetworkAccessManager::finished, this, &OpenSkyFetcher::httpFinished);
// disconnect(mFetchTimer, &QTimer::timeout, this, &OpenSkyFetcher::fetchdata);

// delete mNetMgr;
// delete mFetchTimer;
}
