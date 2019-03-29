#ifndef OPENSKYFETCHER_H
#define OPENSKYFETCHER_H

#include <QThread>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrl>
#include <osgEarth/GeoTransform>
#include <osgEarth/MapNode>
#include <osg/AutoTransform>
#include <osgViewer/View>
#include <plane2d.h>


typedef std::list<osg::ref_ptr<osg::OperationThread>> Threads;

class ThreadHandler: public osg::Operation
{
public:
  ThreadHandler(osgViewer::View *window):
    _window(window)
  {
  }

  virtual void  operator()(osg::Object *obj)
  {
    std::cout << "HI" << std::endl;
  }

private:
  osgViewer::View *_window;
};


class OpenSkyFetcher: public QThread
{
public:
  OpenSkyFetcher(osg::ref_ptr<osgEarth::MapNode> mapnode, osgViewer::View *view);

  void  stopTimer();

  void  run();

public slots:
  void  httpFinished(QNetworkReply *reply);

  void  fetchdata();

  void  httpError(QNetworkReply::NetworkError);

  void  readyRead();

protected:
private:
  Threads                           mOpThreads;
  QUrl                              m_url;
  QTimer                           *mFetchTimer;
  bool                              mIsUnderFetching;
  QNetworkAccessManager            *mNetMgr;
  QNetworkRequest                  *mNetRequest;
  QMap<int, osg::ref_ptr<Plane2D>>  mModels;
  osg::Group                       *_group;
  osg::ref_ptr<osg::Node>           mModel;
  osg::ref_ptr<osg::AutoTransform>  mModel2D;
  osg::ref_ptr<osgEarth::MapNode>   mMapNode;
  osgViewer::View                  *mMainViewer;
  ThreadHandler                    *mHandler;
  osg::Geometry                    *mImage;
};

#endif // OPENSKYFETCHER_H
