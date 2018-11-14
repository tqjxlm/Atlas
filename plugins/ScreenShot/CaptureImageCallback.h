#pragma once

#include <QObject>

#include <osg/Camera>
#include <osgViewer/Renderer>

class CaptureImageCallback : public QObject, public osg::Camera::DrawCallback
{
public:
	CaptureImageCallback(GLenum readBuffer, const std::string& name);

	virtual void operator () (osg::RenderInfo& renderInfo) const;

protected:
	GLenum                      _readBuffer;
	std::string                 _fileName;
	osg::ref_ptr<osg::Image>    _image;
	mutable OpenThreads::Mutex  _mutex;
	static bool first;
};

/** Do Culling only while loading PagedLODs*/
class CustomRenderer : public osgViewer::Renderer
{
public:
	CustomRenderer(osg::Camera* camera);

	/** Set flag to omit drawing in renderingTraversals */
	void setCullOnly(bool on) { _cullOnly = on; }

	virtual void operator () (osg::GraphicsContext* /*context*/);

	virtual void cull();

	bool _cullOnly;
};
