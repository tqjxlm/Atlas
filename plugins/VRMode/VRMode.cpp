#include "VRMode.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QGLWidget>
#include <QMessageBox>

#include <osg/NodeCallback>
#include <osg/CullFace>
#include <osgQt/GraphicsWindowQt>
#include <osg/GraphicsContext>

#include "openvrviewer.h"
#include "openvreventhandler.h"
#include <osgViewer/GraphicsWindow>

#include <ViewerWidget/ViewerWidget.h>
#include <MapController/MapController.h>


class VRControlCallback: public osg::NodeCallback
{
public:
	VRControlCallback(VRMode *plugin):
		plugin(plugin)
	{
	}

	virtual void  operator()(osg::Node *node, osg::NodeVisitor *nv)
	{
		plugin->controlEvent();
	}

	VRMode *plugin;
};

enum TouchpadLocation
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

// Map touchpad angle to 4 directions
inline TouchpadLocation  GetTouchpadLocation(double angle)
{
	if ((angle > 45) && (angle < 135))
	{
		return DOWN;
	}

	if ((angle < -45) && (angle > -135))
	{
		return UP;
	}

	if (((angle < 180) && (angle > 135)) || ((angle < -135) && (angle > -180)))
	{
		return LEFT;
	}

	if (((angle > 0) && (angle < 45)) || ((angle > -45) && (angle < 0)))
	{
		return RIGHT;
	}

	return RIGHT;
}

// Compute vector angle from the front direction
inline double  VectorAngle(osg::Vec2f &v2)
{
	osg::Vec2f  v1(1, 0);
	double      up    = v1 * v2;
	double      down  = v1.length() * v2.length();
	double      cosr  = up / down;
	double      angle = acos(cosr) * 180 / osg::PI;

	if (((v2.x() < 0) && (v2.y() < 0)) || ((v2.x() > 0) && (v2.y() < 0)))
	{
		angle = -angle;
	}

	return angle;
}

VRMode::VRMode():
	_VRView(NULL),
	_openVRDevice(NULL),
	_VRReady(false)
{
	_pluginCategory  = "Effect";
	_pluginName      = "VR Mode";
	_controlCallback = new VRControlCallback(this);
}

VRMode::~VRMode()
{
	removeVRView();
}

void  VRMode::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("VRModeAction"));
	_action->setCheckable(true);
	QIcon  icon1;
	icon1.addFile(QStringLiteral("resources/icons/3dglasses.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon1);
	_action->setText(tr("VR Mode"));
	_action->setToolTip(tr("Start VR Mode"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	toolBar->addAction(_action);
	menu->addAction(_action);
}

void  VRMode::addVRView()
{
	if (_openVRDevice.valid())
	{
		return;
	}

	_VRReady = false;

	// Check if a valid hardware is visible
	if (!OpenVRDevice::hmdPresent())
	{
		osg::notify(osg::FATAL) << "Error: No valid HMD present!" << std::endl;
		emit  aborted();

		return;
	}

	// Set up openvr device
	float  nearClip           = 0.01f;
	float  farClip            = 10000.0f;
	float  worldUnitsPerMetre = 1.0f;
	int    samples            = 4;
	_openVRDevice = new OpenVRDevice(nearClip, farClip, worldUnitsPerMetre, samples);

	if (!_openVRDevice->hmdInitialized())
	{
		osg::notify(osg::FATAL) << "Error: Fail to init HMD!" << std::endl;
		emit  aborted();

		return;
	}

	// Create graphics context
	osg::ref_ptr<osg::GraphicsContext::Traits>  traits = _openVRDevice->graphicsContextTraits();
	traits->windowName    = "VRMode";
	traits->sharedContext = _mainViewer->getMainContext();
	_VRGraphicsWindow     = new osgQt::GraphicsWindowQt(traits.get());
	osg::GraphicsContext::incrementContextIDUsageCount(_VRGraphicsWindow->getState()->getContextID());
	_VRContext = _VRGraphicsWindow;

	if (!_VRContext.valid())
	{
		osg::notify(osg::FATAL) << "Error, GraphicsWindow has not been created successfully" << std::endl;
		emit  aborted();

		return;
	}

	// Create a view and a widget
	osg::ref_ptr<OpenVRRealizeOperation>  openvrRealizeOperation = new OpenVRRealizeOperation(_openVRDevice);
	_mainViewer->setRealizeOperation(openvrRealizeOperation.get());
	_mainViewer->setReleaseContextAtEndOfFrameHint(false);

	_VRWidget = _mainViewer->createViewWidget(_VRGraphicsWindow, _root);
	_VRWidget->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

	_VRView = _mainViewer->getView(_mainViewer->getNumViews() - 1);
	_VRView->addEventHandler(new OpenVREventHandler(_openVRDevice));

	// Set VRViewer
	osg::ref_ptr<OpenVRViewer>  openvrViewer = new OpenVRViewer(_VRView, _openVRDevice, openvrRealizeOperation);
	openvrViewer->addChild(_root);
	openvrViewer->addEventCallback(_controlCallback);
	_VRView->setSceneData(openvrViewer);
	_mainViewer->realize();
	_VRView->setCameraManipulator(_mainViewer->getMainView()->getCameraManipulator(), false);

	// Its optional to actually show the window, VR will work without showing it
	_VRWidget->show();
	_VRReady = true;
}

bool  VRMode::isVRReady()
{
	return _VRReady;
}

bool  VRMode::isVRRunning()
{
	return _openVRDevice.valid();
}

void  VRMode::removeVRView()
{
	if (!_openVRDevice.valid())
	{
		return;
	}

	_VRWidget->close();
	_VRView->setCamera(NULL);

	_openVRDevice->shutdown(_VRContext);
	_mainViewer->removeView(_VRView);

	_VRGraphicsWindow = NULL;
	_VRContext        = NULL;
	_openVRDevice     = NULL;
}

void  VRMode::controlEvent()
{
	// If a vr device is available, convert its controllers' inputs to the viewer
	if (_activated && _openVRDevice.valid())
	{
		if (_openVRDevice->controllerEventResult != -1)
		{
			// Generate osgGA events
			osg::ref_ptr<osgGA::GUIEventAdapter>  controllerBeforeEvent = new osgGA::GUIEventAdapter;
			osg::ref_ptr<osgGA::GUIEventAdapter>  controllerEvent       = new osgGA::GUIEventAdapter;
			osg::ref_ptr<osgGA::GUIEventAdapter>  controllerAfterEvent  = new osgGA::GUIEventAdapter;

			controllerBeforeEvent->setEventType(osgGA::GUIEventAdapter::PUSH);
			controllerAfterEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);

			// Convert controller inputs
			switch (_openVRDevice->controllerEventResult)
			{
			case OpenVRDevice::Touchpad_Press:
			{
				_clear = false;
				double            angle    = VectorAngle(_openVRDevice->m_touchpadTouchPosition);
				TouchpadLocation  location = GetTouchpadLocation(angle);
				controllerEvent->setEventType(osgGA::GUIEventAdapter::DRAG);

				if (location == UP)
				{
					if (_trigger)
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
					}
					else
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
					}

					controllerEvent->setY(_fake_position_y);
					_fake_position_y++;
				}
				else if (location == DOWN)
				{
					if (_trigger)
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON);
					}
					else
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
					}

					controllerEvent->setY(_fake_position_y);
					_fake_position_y--;
				}
				else if (location == LEFT)
				{
					if (_trigger)
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
					}
					else
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
					}

					controllerEvent->setX(_fake_position_x);

					_fake_position_x--;
				}
				else if (location == RIGHT)
				{
					if (_trigger)
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON);
					}
					else
					{
						controllerEvent->setButtonMask(osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON);
					}

					controllerEvent->setX(_fake_position_x);
					_fake_position_x++;
				}
			}

			break;
			case OpenVRDevice::Touchpad_Unpress:
				controllerEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				controllerEvent->setButtonMask(0);
				controllerEvent->setX(_fake_position_x);
				controllerEvent->setY(_fake_position_y);
				_fake_position_x                     = 0;
				_fake_position_y                     = 0;
				_openVRDevice->controllerEventResult = -1;

				controllerBeforeEvent->setEventType(osgGA::GUIEventAdapter::PUSH);
				controllerAfterEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				_clear = true;

				break;
			case OpenVRDevice::Trigger_Press:
				_clear   = false;
				_trigger = true;

				break;
			case OpenVRDevice::Trigger_Unpress:
				_trigger = false;
				controllerEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				controllerEvent->setButtonMask(0);
				controllerEvent->setX(_fake_position_x);
				controllerEvent->setY(_fake_position_y);
				_fake_position_x                     = 0;
				_fake_position_y                     = 0;
				_openVRDevice->controllerEventResult = -1;

				controllerBeforeEvent->setEventType(osgGA::GUIEventAdapter::PUSH);
				controllerAfterEvent->setEventType(osgGA::GUIEventAdapter::RELEASE);
				_clear = true;

				break;
			default:
				break;
			}

			// Insert events into event queue
			_VRGraphicsWindow->getEventQueue()->addEvent(controllerEvent);

			if (_clear)
			{
				_VRGraphicsWindow->getEventQueue()->addEvent(controllerBeforeEvent);
				_VRGraphicsWindow->getEventQueue()->addEvent(controllerAfterEvent);
			}
		}

		// Handle 'exit' events
		if (_VRGraphicsWindow.valid() && _VRGraphicsWindow->checkEvents())
		{
			osgGA::EventQueue::Events  events;
			_VRGraphicsWindow->getEventQueue()->copyEvents(events);
			osgGA::EventQueue::Events::iterator  itr;

			for (itr = events.begin(); itr != events.end(); ++itr)
			{
				osgGA::GUIEventAdapter *event = dynamic_cast<osgGA::GUIEventAdapter *>(itr->get());

				if ((event != nullptr) && (event->getEventType() == osgGA::GUIEventAdapter::CLOSE_WINDOW))
				{
					events.erase(itr);
					_VRGraphicsWindow->getEventQueue()->setEvents(events);
					_VRGraphicsWindow->getEventQueue()->quitApplication();
					break;
				}
			}
		}
	}
}

void  VRMode::toggle(bool checked /*= true*/)
{
	_activated = checked;

	if (checked)
	{
		addVRView();

		if (!isVRReady())
		{
			QMessageBox::critical(0, tr("Error"), tr("VR device initialization failed"));
		}
	}
	else
	{
		if (isVRRunning())
		{
			removeVRView();
		}
	}
}
