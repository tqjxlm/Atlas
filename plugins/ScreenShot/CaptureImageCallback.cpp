#include "CaptureImageCallback.h"

#include <QMessageBox>

#include <osgDB/WriteFile>
#include <osgViewer/View>

bool CaptureImageCallback::first = false;

CaptureImageCallback::CaptureImageCallback(GLenum readBuffer, const std::string& name)
	: _readBuffer(readBuffer)
	, _fileName(name)
{
	_image = new osg::Image;
	first = false;
}

void CaptureImageCallback::operator()(osg::RenderInfo& renderInfo) const
{
	if (!first)
	{
		glReadBuffer(_readBuffer);
		OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
		osg::GraphicsContext* gc = renderInfo.getState()->getGraphicsContext();
		if (gc->getTraits())
		{
			GLenum pixelFormat;

			if (gc->getTraits()->alpha)
				pixelFormat = GL_RGBA;
			else
				pixelFormat = GL_RGB;

			int width = gc->getTraits()->width;
			int height = gc->getTraits()->height;

			_image->readPixels(0, 0, width, height, pixelFormat, GL_UNSIGNED_BYTE);
		}

		if (!_fileName.empty())
		{
			osgDB::writeImageFile(*_image, _fileName);
			first = true;
			QMessageBox::about(NULL, "note", tr("Screenshot OK."));
		}
	}
}

CustomRenderer::CustomRenderer(osg::Camera* camera)
	: osgViewer::Renderer(camera)
	, _cullOnly(true)
{
}

void CustomRenderer::operator()(osg::GraphicsContext* /*context*/)
{
	if (_graphicsThreadDoesCull)
	{
		if (_cullOnly)
			cull();
		else
			cull_draw();
	}
}

void CustomRenderer::cull()
{
	osgUtil::SceneView* sceneView = _sceneView[0].get();
	if (!sceneView || _done) return;

	updateSceneView(sceneView);

	osgViewer::View* view = dynamic_cast<osgViewer::View*>(_camera->getView());
	if (view) sceneView->setFusionDistance(view->getFusionDistanceMode(), view->getFusionDistanceValue());

	sceneView->inheritCullSettings(*(sceneView->getCamera()));
	sceneView->cull();
}
