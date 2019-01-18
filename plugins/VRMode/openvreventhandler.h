/*
 * openvreventhandler.h
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 */

#ifndef _OSG_OPENVREVENTHANDLER_H_
#define _OSG_OPENVREVENTHANDLER_H_

#include <osgViewer/ViewerEventHandlers>

// Forward declaration
class OpenVRDevice;


class OpenVREventHandler: public osgGA::GUIEventHandler
{
public:
	explicit OpenVREventHandler(osg::ref_ptr<OpenVRDevice> device):
		m_openvrDevice(device), m_usePositionalTracking(true)
	{
	}

	virtual bool  handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter&);

protected:
	osg::ref_ptr<OpenVRDevice>  m_openvrDevice;
	bool                        m_usePositionalTracking;
};

#endif /* _OSG_OPENVREVENTHANDLER_H_ */
