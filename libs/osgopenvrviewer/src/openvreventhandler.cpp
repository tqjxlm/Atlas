/*
 * openvreventhandler.cpp
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 */

#include <iostream>
#include "openvreventhandler.h"
#include "openvrdevice.h"

bool OpenVREventHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& ad)
{
    switch (ea.getEventType())
    {
        case osgGA::GUIEventAdapter::KEYUP:
        {
            switch (ea.getKey())
            {
                case osgGA::GUIEventAdapter::KEY_R:
                    m_openvrDevice->resetSensorOrientation();
                    break;
				case osgGA::GUIEventAdapter::KEY_B:
					std::cout << "Keyborad B" << std::endl;
            }
        }
		break;
		case(osgGA::GUIEventAdapter::DRAG):
		{
			//std::cout << ea.getX() << "," << ea.getY() << "," << ea.getXnormalized() << "," << ea.getYnormalized() << std::endl;
			return false;
		}
    }

    return osgGA::GUIEventHandler::handle(ea, ad);
}
