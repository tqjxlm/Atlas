#include "osghudvr.h"
#include "openvrdevice.h"
#include <osgText/Text>
#include <osg/PolygonOffset>

OSGHudVR::OSGHudVR()
{
    createHud();
}

osg::Camera *OSGHudVR::createHudLeft(osg::GraphicsContext *gc, OpenVRDevice *device, osg::Vec4 clearColor)
{
    OpenVRTextureBuffer* buffer = device->getTextureBufferLeft();

    osg::ref_ptr<osg::Camera> camera = new osg::Camera();
    camera->setClearColor(clearColor);
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER, OpenVRDevice::LEFT);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setAllowEventFocus(false);
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF_INHERIT_VIEWPOINT);
    camera->setViewport(0, 0, buffer->textureWidth(), buffer->textureHeight());
    camera->setGraphicsContext(gc);

    // Here we avoid doing anything regarding OSG camera RTT attachment.
    // Ideally we would use automatic methods within OSG for handling RTT but in this
    // case it seemed simpler to handle FBO creation and selection within this class.

    // This initial draw callback is used to disable normal OSG camera setup which
    // would undo our RTT FBO configuration.
    camera->setInitialDrawCallback(new OpenVRInitialDrawCallback());

    camera->setPreDrawCallback(new OpenVRPreDrawCallback(camera.get(), buffer));
    camera->setFinalDrawCallback(new OpenVRPostDrawCallback(camera.get(), buffer));
    camera->setRenderOrder(osg::Camera::POST_RENDER, OpenVRDevice::LEFT);
    // only clear the depth buffer !!!
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,1080,0,1200) * device->projectionOffsetMatrixLeft());
    return camera.release();

}

osg::Camera *OSGHudVR::createHudRight(osg::GraphicsContext *gc, OpenVRDevice *device, osg::Vec4 clearColor)
{
    OpenVRTextureBuffer* buffer = device->getTextureBufferRight();

    osg::ref_ptr<osg::Camera> camera = new osg::Camera();
    camera->setClearColor(clearColor);
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER, OpenVRDevice::RIGHT);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setAllowEventFocus(false);
    camera->setReferenceFrame(osg::Camera::ABSOLUTE_RF_INHERIT_VIEWPOINT);
    camera->setViewport(0, 0, buffer->textureWidth(), buffer->textureHeight());
    camera->setGraphicsContext(gc);

    // Here we avoid doing anything regarding OSG camera RTT attachment.
    // Ideally we would use automatic methods within OSG for handling RTT but in this
    // case it seemed simpler to handle FBO creation and selection within this class.

    // This initial draw callback is used to disable normal OSG camera setup which
    // would undo our RTT FBO configuration.
    camera->setInitialDrawCallback(new OpenVRInitialDrawCallback());

    camera->setPreDrawCallback(new OpenVRPreDrawCallback(camera.get(), buffer));
    camera->setFinalDrawCallback(new OpenVRPostDrawCallback(camera.get(), buffer));
    camera->setRenderOrder(osg::Camera::POST_RENDER, OpenVRDevice::RIGHT);
    // only clear the depth buffer !!!
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,1080,0,1200) * device->projectionOffsetMatrixRight());
    return camera.release();

}

osg::Geode* OSGHudVR::createHud()
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();

    std::string timesFont("fonts/arial.ttf");

    // turn lighting off for the text and disable depth test to ensure it's always ontop.
    osg::StateSet* stateset = geode->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

    osg::Vec3 position(150.0f,800.0f,-0.5f);
    osg::Vec3 delta(0.0f,-120.0f,0.0f);

    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("Head Up Displays are simple :-)");

        position += delta;
    }


    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("All you need to do is create your text in a subgraph.");

        position += delta;
    }


    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("Then place an osg::Camera above the subgraph\n"
                      "to create an orthographic projection.\n");

        position += delta;
    }

    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("Set the Camera's ReferenceFrame to ABSOLUTE_RF to ensure\n"
                      "it remains independent from any external model view matrices.");

        position += delta;
    }

    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("And set the Camera's clear mask to just clear the depth buffer.");

        position += delta;
    }

    {
        osgText::Text* text = new  osgText::Text;
        geode->addDrawable( text );

        text->setFont(timesFont);
        text->setPosition(position);
        text->setText("And finally set the Camera's RenderOrder to POST_RENDER\n"
                      "to make sure it's drawn last.");

        position += delta;
    }


    {
        osg::BoundingBox bb;
        for(unsigned int i=0;i<geode->getNumDrawables();++i)
        {
            bb.expandBy(geode->getDrawable(i)->getBoundingBox());
        }

        osg::Geometry* geom = new osg::Geometry;

        osg::Vec3Array* vertices = new osg::Vec3Array;
        float depth = bb.zMin()-0.1;
        vertices->push_back(osg::Vec3(bb.xMin(),bb.yMax(),depth));
        vertices->push_back(osg::Vec3(bb.xMin(),bb.yMin(),depth));
        vertices->push_back(osg::Vec3(bb.xMax(),bb.yMin(),depth));
        vertices->push_back(osg::Vec3(bb.xMax(),bb.yMax(),depth));
        geom->setVertexArray(vertices);

        osg::Vec3Array* normals = new osg::Vec3Array;
        normals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
        geom->setNormalArray(normals, osg::Array::BIND_OVERALL);

        osg::Vec4Array* colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.0f,1.0,0.8f,0.2f));
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);

        geom->addPrimitiveSet(new osg::DrawArrays(GL_QUADS,0,4));

        osg::StateSet* stateset = geom->getOrCreateStateSet();
        stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
        stateset->setAttribute(new osg::PolygonOffset(1.0f,1.0f),osg::StateAttribute::ON);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        geode->addDrawable(geom);
    }
    return geode.release();
}
