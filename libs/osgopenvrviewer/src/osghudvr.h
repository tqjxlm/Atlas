#ifndef OSGHUDVR_H
#define OSGHUDVR_H

#include <osg/Camera>

class OpenVRDevice;

class OSGHudVR
{
public:
    OSGHudVR();
    static osg::Camera *createHudLeft(osg::GraphicsContext *gc, OpenVRDevice *device, osg::Vec4 clearColor);
    static osg::Camera *createHudRight(osg::GraphicsContext *gc, OpenVRDevice *device, osg::Vec4 clearColor);
    static osg::Geode* createHud();
};

#endif // OSGHUDVR_H
