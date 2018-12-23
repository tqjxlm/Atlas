#include "MapController.h"

#include <QTime>
#include <QDebug>

#include <osgViewer/View>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Matrix>

#include <osgEarth/SpatialReference>

using namespace osg;

const double  ZERO_LIMIT   = 0.0001;
const double  MIN_NF_RATIO = 0.000001;
const double  MAX_NF_RATIO = 0.0001;

inline float  computeAngle(const osg::Vec3 &endVector, const osg::Vec3 &startVector)
{
	osg::Vec3  startXY(startVector.x(), startVector.y(), 0.0);
	osg::Vec3  endXY(endVector.x(), endVector.y(), 0.0);

	startXY.normalize();
	endXY.normalize();

	float  angle = acos(startXY * endXY);

	return (startXY ^ endXY).z() > 0 ? angle : -angle;
}

inline float  computeRoll(const osg::Vec3 &endVector, const osg::Vec3 &startVector)
{
	osg::Vec3  v1 = startVector;
	osg::Vec3  v2 = endVector;

	v1.normalize();
	v2.normalize();

	return asin(v2.z()) - asin(v1.z());
}

MapController::MapController(osg::ref_ptr<osg::Node> dataRoot, osg::ref_ptr<osg::Node> mapRoot, const osgEarth::SpatialReference *srs):
	_dataRoot(dataRoot),
	_mapRoot(mapRoot),
	_interval(2),
	_totalTime(1),
	_totalSteps(_totalTime / (1 / 60.0 * _interval)),
	_inAnimation(false),
	_isFlying(false),
	_isDriving(false),
	_isScreenSaving(false),
	_centerIndicator(NULL),
	_srs(srs)
{
	setAllowThrow(false);
	setVerticalAxisFixed(true);
	setMinimumDistance(0.000001);
	setRelativeFlag(_minimumDistanceFlagIndex, true);

	for (auto &view : _views)
	{
		view = nullptr;
	}
}

bool  MapController::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us)
{
	// In an animation, don't accept events
	if (_inAnimation)
	{
		return true;
	}

	switch (ea.getEventType())
	{
	case (osgGA::GUIEventAdapter::PUSH):

		// Show camera indicator
		if (_centerIndicator.valid())
		{
			_centerIndicator->setNodeMask(0xffffffff);
		}

		break;
	case (osgGA::GUIEventAdapter::RELEASE):

		// Hide camera indicator
		if (_centerIndicator.valid())
		{
			_centerIndicator->setNodeMask(0);
		}

		break;
	case (osgGA::GUIEventAdapter::KEYDOWN):

		// Keyboard: don't pass the event on
		return handleKeyDown(ea, us);
	case (osgGA::GUIEventAdapter::KEYUP):

		// Keyboard: don't pass the event on
		return handleKeyUp(ea, us);
	default:
		break;
	}

	// Hand over the event to the default pipeline
	return OrbitManipulator::handle(ea, us);
}

void  MapController::updateCamera(osg::Camera &camera)
{
	OrbitManipulator::updateCamera(camera);

	// If a flying action is scheduled, move according to the settings
	if (_inAnimation && (++_timeStamp == _interval))
	{
		// The flying action is performed in three stages
		if (_isFlying)
		{
			flyingMovement();
		}
		else if (_isScreenSaving)
		{
			screenSaversMovement();
		}
		else
		{
			_inAnimation = false;
		}

		_timeStamp = 0;
	}

	if (_isDriving)
	{
		drivingMovement();
	}

	// The camera indicator should always be at the center
	if (_centerIndicator.valid())
	{
		_centerIndicator->setPosition(_center);
	}
}

void  MapController::toggleAnimation(bool enabled)
{
	_inAnimation = enabled;
}

void  MapController::flyToPoint(osg::Vec3 targetPosition, float targetDistance)
{
	// Set up target
	_targetPosition = targetPosition;

	// Set up speed
	_acceleration = (targetPosition - getCenter()) * 4 / pow(_totalSteps, 2);
	_speed        = osg::Vec3();

	// Set action flags
	toggleAnimation(true);
	_stepCount = 0;
	_timeStamp = 0;
	_isFlying  = true;
}

void  MapController::flyingMovement()
{
	// A simple speed-up / slow-down scheme
	_stepCount++;

	if (_stepCount <= _totalSteps / 2)
	{
		_speed += _acceleration;
	}
	else
	{
		_speed -= _acceleration;
	}

	if (_stepCount <= _totalSteps)
	{
		setCenter(getCenter() + _speed);
	}
	else
	{
		_isFlying = false;
	}
}

void  MapController::drivingMovement()
{
	_driveSpeed = getDistance() / 500;
	_center    += _driveDirection * _driveSpeed;

	stickToScene();
}

void  MapController::setCenterIndicator(osg::PositionAttitudeTransform *center)
{
	_centerIndicator = center;
}

void  MapController::fitViewOnNode(const osg::Node *scene, double addHeight)
{
	// Get local bounding
	osg::BoundingSphere  bs       = scene->getBound();
	osg::Vec3            bsCenter = bs.center();

	// Calculate world transformation
	// FIXME: Only the first parent is taken into account here, which may not be the case.
	osg::Vec3    transCenter = osg::Vec3(0, 0, 0);
	osg::Matrix  worldMatrix = osg::computeLocalToWorld(scene->getParentalNodePaths()[0]);
	osg::Vec3    trans       = worldMatrix.getTrans();

	transCenter = bsCenter + trans;

	// Subtract the self transformation if any
	const osg::PositionAttitudeTransform *pat = dynamic_cast<const osg::PositionAttitudeTransform *>(scene);

	if (pat)
	{
		osg::Vec3  selfTrans = pat->getPosition();
		transCenter = transCenter - selfTrans;
	}

	bs.center() = transCenter;

	// Zoom view to overlook the new bounding
	fitViewOnBounding(&bs, addHeight);
}

void  MapController::fitViewOnBounding(const osg::BoundingSphere *bs, double addHeight)
{
	setCenter(osg::Vec3(bs->center().x(), bs->center().y(), bs->center().z()));
	setDistance(bs->radius() * 3);
	setHeading(ZERO_LIMIT);
	setElevation(osg::PI / 2 - ZERO_LIMIT);
	osg::Vec3d  eye, center, up;
	getTransformation(eye, center, up);
	setHomePosition(eye, center, up);
}

void  MapController::setViewPoint(const osgEarth::Viewpoint &vp)
{
	if (!vp.isValid())
	{
		return;
	}

	double  distance = vp.range().value().getValue();

	if (vp.nodeIsSet())
	{
		fitViewOnNode(vp.getNode(), distance);
	}
	else
	{
		osgEarth::GeoPoint  globalPos;

		if (vp.focalPoint().value().transform(_srs, globalPos))
		{
			flyToPoint(globalPos.vec3d(), distance);
		}
	}
}

bool  MapController::handleMouseWheel(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us)
{
	osgGA::GUIEventAdapter::ScrollingMotion  sm = ea.getScrollingMotion();

	// handle centering
	if (_flags & SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT)
	{
		if ((((sm == osgGA::GUIEventAdapter::SCROLL_DOWN) && (_wheelZoomFactor > 0.)))
		    || (((sm == osgGA::GUIEventAdapter::SCROLL_UP) && (_wheelZoomFactor < 0.))))
		{
			if (getAnimationTime() <= 0.)
			{
				// center by mouse intersection (no animation)
				setCenterByMousePointerIntersection(ea, us);
			}
			else
			{
				// start new animation only if there is no animation in progress
				if (!isAnimating())
				{
					startAnimationByMousePointerIntersection(ea, us);
				}
			}
		}
	}

	switch (sm)
	{
	// mouse scroll up event
	case osgGA::GUIEventAdapter::SCROLL_UP:
	{
		// perform zoom
		zoomModel(-_wheelZoomFactor, true);
		us.requestRedraw();
		us.requestContinuousUpdate(isAnimating() || _thrown);

		return true;
	}

	// mouse scroll down event
	case osgGA::GUIEventAdapter::SCROLL_DOWN:
	{
		// perform zoom
		zoomModel(_wheelZoomFactor, false);
		us.requestRedraw();
		us.requestContinuousUpdate(isAnimating() || _thrown);

		return true;
	}

	// unhandled mouse scrolling motion
	default:

		return false;
	}
}

bool  MapController::performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
	return OrbitManipulator::performMovementLeftMouseButton(eventTimeDelta, dx, dy);
}

bool  MapController::performMovementLeftMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
	// Pane action
	float   scale = -0.5f * _distance * getThrowScale(eventTimeDelta);
	Matrix  rotation_matrix;

	rotation_matrix.makeRotate(_rotation);

	Vec3d  dv = Vec3d(dx * scale, dy * scale, 0) * rotation_matrix;

	_center += dv;

	stickToScene();

	return true;
}

void  MapController::zoomModel(const float dy, bool pushForwardIfNeeded)
{
	// scale
	float  scale = 1.0f + dy;

	// minimum distance
	float  minDist = _minimumDistance;

	if (getRelativeFlag(_minimumDistanceFlagIndex))
	{
		minDist *= _modelSize;
	}

	if (_distance * scale > minDist)
	{
		// regular zoom
		_distance *= scale;
	}
	else
	{
		if (pushForwardIfNeeded)
		{
			// push the camera forward
			Vec3d  eye, center, up;
			getTransformation(eye, center, up);
			center += (center - eye) * 500;

			osg::ref_ptr<osgUtil::LineSegmentIntersector>  ls = new osgUtil::LineSegmentIntersector(eye, center);
			osgUtil::IntersectionVisitor                   iv(ls);

			if (_views[0])
			{
				_views[0]->getCamera()->accept(iv);
				auto  newCenter = ls->getFirstIntersection().getWorldIntersectPoint();
				_distance += (newCenter - _center).length();
				_center    = newCenter;
			}
		}
		else
		{
			// set distance on its minimum value
			_distance = minDist;
		}
	}
}

void  MapController::rotateWithFixedVertical(const float dx, const float dy)
{
	CoordinateFrame  coordinateFrame = getCoordinateFrame(m_center);
	Vec3d            localUp         = getUpVector(coordinateFrame);

	for (auto view : _views)
	{
		if (view)
		{
			if (getElevation() > osg::DegreesToRadians(-25.0))
			{
				view->getCamera()->setNearFarRatio(MIN_NF_RATIO);
			}
			else
			{
				view->getCamera()->setNearFarRatio(MAX_NF_RATIO);
			}
		}
	}

	// Keep elevation above certain value
	if ((getElevation() > 0) && (dy > 0))
	{
		rotateYawPitch(_rotation, dx, 0, localUp);
	}
	else
	{
		rotateYawPitch(_rotation, dx, dy, localUp);
	}
}

bool  MapController::handleKeyDown(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us)
{
	if (!_isDriving)
	{
		if (dynamic_cast<const osgViewer::View *>(&ea) == _views[0])
		{
			stickToScene();
		}

		_isDriving = true;

		Vec3d  eye, center, up;
		getTransformation(eye, center, up);
		_front     = center - eye;
		_front.z() = 0;
		_front.normalize();

		_right = _front ^ osg::Vec3(0, 0, 1);
	}

	if (_isDriving)
	{
		switch (ea.getKey())
		{
		case (osgGA::GUIEventAdapter::KEY_Up):
		case (osgGA::GUIEventAdapter::KEY_Down):
		case (osgGA::GUIEventAdapter::KEY_Left):
		case (osgGA::GUIEventAdapter::KEY_Right):

			if (!_keyPressed[ea.getKey()])
			{
				_keyPressed[ea.getKey()] = true;
				updateDriveDirection();
			}

			break;
		default:
			break;
		}
	}

	return false;
}

bool  MapController::handleKeyUp(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us)
{
	if (_isDriving)
	{
		switch (ea.getKey())
		{
		case (osgGA::GUIEventAdapter::KEY_Up):
		case (osgGA::GUIEventAdapter::KEY_Down):
		case (osgGA::GUIEventAdapter::KEY_Left):
		case (osgGA::GUIEventAdapter::KEY_Right):
			_keyPressed[ea.getKey()] = false;

			updateDriveDirection();

			break;
		default:
			break;
		}

		// Stop moving if all keys released
		for (auto pressed : _keyPressed)
		{
			if (pressed)
			{
				return false;
			}
		}

		_isDriving = false;
	}

	return false;
}

void  MapController::stickToScene()
{
	Vec3d  eye, center, up;

	getTransformation(eye, center, up);
	center += (center - eye) * 500;

	// Check if the view direction intersects with the data root
	osg::ref_ptr<osgUtil::LineSegmentIntersector>  ls = new osgUtil::LineSegmentIntersector(eye, center);
	osgUtil::IntersectionVisitor                   iv(ls);
	iv.setTraversalMask(INTERSECT_IGNORE);

	if (_views[0] != NULL)
	{
		_views[0]->getCamera()->accept(iv);

		if (!ls->getIntersections().empty())
		{
			// If intersected, change the trackball center to the intersected point
			auto  newCenter = ls->getIntersections().begin()->getWorldIntersectPoint();
			_distance = (newCenter - eye).length();
			_center   = newCenter;

			return;
		}
	}
}

void  MapController::updateDriveDirection()
{
	_driveDirection = { 0, 0, 0 };

	if (_keyPressed[osgGA::GUIEventAdapter::KEY_Up])
	{
		_driveDirection += _front;
	}

	if (_keyPressed[osgGA::GUIEventAdapter::KEY_Down])
	{
		_driveDirection -= _front;
	}

	if (_keyPressed[osgGA::GUIEventAdapter::KEY_Left])
	{
		_driveDirection -= _right;
	}

	if (_keyPressed[osgGA::GUIEventAdapter::KEY_Right])
	{
		_driveDirection += _right;
	}

	_driveDirection.normalize();
}

void  MapController::registerWithView(osgViewer::View *view, int index)
{
	_views[index] = view;
}

// screensavers
void  MapController::screenSaversMovement()
{
	// Rotate around longitude direction
	setHeading(getHeading() + 0.002);
}

void  MapController::toggleScreenSaving(bool on)
{
	_inAnimation    = on;
	_isScreenSaving = on;
}
