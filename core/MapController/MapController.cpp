#include "MapController.h"

#include <QTime>
#include <QDebug>

#include <osgViewer/View>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <osg/Matrix>

using namespace osg;

const double ZERO_LIMIT = 0.0001;

inline float computeAngle(const osg::Vec3& endVector, const osg::Vec3& startVector)
{
	osg::Vec3 startXY(startVector.x(), startVector.y(), 0.0);
	osg::Vec3 endXY(endVector.x(), endVector.y(), 0.0);
	startXY.normalize();
	endXY.normalize();

	float angle = acos(startXY * endXY);
	return (startXY ^ endXY).z() > 0 ? angle : -angle;
}

inline float computeRoll(const osg::Vec3& endVector, const osg::Vec3& startVector)
{
	osg::Vec3 v1 = startVector;
	osg::Vec3 v2 = endVector;
	v1.normalize();
	v2.normalize();

	return asin(v2.z()) - asin(v1.z());
}

MapController::MapController(void)
	: _interval(2)
	, _totalTime(1)
	, _totalSteps(_totalTime / (1 / 60.0 * _interval))
	, _isTraveling(false)
	, _isFlying(false)
	, _isScreenSaving(false)
	, _centerIndicator(NULL)
	, _view(NULL)
{
	setAllowThrow(false);
	setVerticalAxisFixed(true);
	setMinimumDistance(0.000001);
	setRelativeFlag(_minimumDistanceFlagIndex, true);
}

bool MapController::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	osgViewer::View* viewer = dynamic_cast<osgViewer::View*>(&us);
	_view = viewer;

	osgUtil::LineSegmentIntersector::Intersections intersections;

	switch (ea.getEventType())
	{
	case(osgGA::GUIEventAdapter::PUSH):
		// Show camera indicator
		_centerIndicator->setNodeMask(0xffffffff);
		if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
		{
			if (_isTraveling == true && viewer->computeIntersections(ea, intersections))
			{
				typedef osgUtil::LineSegmentIntersector::Intersections::const_iterator itr_Intersection;
				itr_Intersection first = intersections.begin();
				flyToPoint(first->getWorldIntersectPoint(), first->getWorldIntersectNormal());
			}
		}
		break;
	case(osgGA::GUIEventAdapter::RELEASE):
		// Hide camera indicator
		_centerIndicator->setNodeMask(0);
		break;
	default:
		break;
	}

	// Hand over the event to the default pipeline
	OrbitManipulator::handle(ea, us);
	return false;
}

void MapController::updateCamera(osg::Camera &camera)
{
	OrbitManipulator::updateCamera(camera);

	// If a flying action is scheduled, move according to the settings
	if (_isTraveling && ++_timeStamp == _interval)
	{
		// The flying action is performed in three stages
		if (_isFlying)
			flyingMovement();
		else if (_isScreenSaving)
			screenSaversMovement();
		_timeStamp = 0;
	}
	
	// The camera indicator should always be at the center
	if (_centerIndicator.valid())
		_centerIndicator->setPosition(_center);
}

void MapController::updateTravelrole(bool isTravel)
{
	_isTraveling = isTravel;
}

void MapController::flyToPoint(osg::Vec3 targetPosition, osg::Vec3 targetNormal, float targetDistance)
{
	// Set up target
	_targetPosition = targetPosition;
	
	// Set up speed
	_acceleration = (targetPosition - getCenter()) * 4 / pow(_totalSteps, 2);
	_speed = osg::Vec3();

	// Set action flags
	updateTravelrole(true);
	_stepCount = 0;
	_timeStamp = 0;
	_isFlying = true;
}

void MapController::flyingMovement()
{
	// A simple speed-up / slow-down scheme
	_stepCount++;
	if (_stepCount <= _totalSteps / 2)
		_speed += _acceleration;
	else
		_speed -= _acceleration;

	if (_stepCount <= _totalSteps)
		setCenter(getCenter() + _speed);
	else
		_isFlying = false;
}

void MapController::setCenterIndicator(osg::PositionAttitudeTransform* center)
{
	_centerIndicator = center;
}

void MapController::fitViewOnNode(const osg::Node* scene, double addHeight)
{
	// Get local bounding
	osg::BoundingSphere bs = scene->getBound();
	osg::Vec3 bsCenter = bs.center();

	// Calculate world transformation
	// FIXME: Only the first parent is taken into account here, which may not be the case.
	osg::Vec3 transCenter = osg::Vec3(0, 0, 0);
	osg::Matrix worldMatrix = osg::computeLocalToWorld(scene->getParentalNodePaths()[0]);
	osg::Vec3 trans = worldMatrix.getTrans();
	transCenter = bsCenter + trans;

	// Subtract the self transformation if any
	const osg::PositionAttitudeTransform* pat = dynamic_cast<const osg::PositionAttitudeTransform*>(scene);
	if (pat)
	{
		osg::Vec3 selfTrans = pat->getPosition();
		transCenter = transCenter - selfTrans;
	}

	bs.center() = transCenter;

	// Zoom view to overlook the new bounding
	fitViewOnBounding(&bs, addHeight);
}

void MapController::fitViewOnBounding(const osg::BoundingSphere* bs, double addHeight)
{
	setCenter(osg::Vec3(bs->center().x(), bs->center().y(), bs->center().z()));
	setDistance(bs->radius() * 3 + addHeight);
	setHeading(ZERO_LIMIT);
	setElevation(osg::PI/2 - ZERO_LIMIT);
	osg::Vec3d eye, center, up;
	getTransformation(eye, center, up);
	setHomePosition(eye, center, up);
}

bool MapController::handleMouseWheel( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us )
{
	osgGA::GUIEventAdapter::ScrollingMotion sm = ea.getScrollingMotion();

	// handle centering
	if( _flags & SET_CENTER_ON_WHEEL_FORWARD_MOVEMENT )
	{
		if( ((sm == osgGA::GUIEventAdapter::SCROLL_DOWN && _wheelZoomFactor > 0.)) ||
			((sm == osgGA::GUIEventAdapter::SCROLL_UP   && _wheelZoomFactor < 0.)) )
		{
			if( getAnimationTime() <= 0. )
			{
				// center by mouse intersection (no animation)
				setCenterByMousePointerIntersection( ea, us );
			}
			else
			{
				// start new animation only if there is no animation in progress
				if( !isAnimating() )
					startAnimationByMousePointerIntersection( ea, us );
			}
		}
	}

	switch( sm )
	{
		// mouse scroll up event
	case osgGA::GUIEventAdapter::SCROLL_UP:
		{
			// perform zoom
			zoomModel( -_wheelZoomFactor, true );
			us.requestRedraw();
			us.requestContinuousUpdate( isAnimating() || _thrown );
			return true;
		}

		// mouse scroll down event
	case osgGA::GUIEventAdapter::SCROLL_DOWN:
		{
			// perform zoom
			zoomModel( _wheelZoomFactor, false );
			us.requestRedraw();
			us.requestContinuousUpdate( isAnimating() || _thrown );
			return true;
		}

		// unhandled mouse scrolling motion
	default:
		return false;
	}
}

bool MapController::performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy)
{
	// Pane action
	float scale = -0.5f * _distance * getThrowScale(eventTimeDelta);

	Matrix rotation_matrix;
	rotation_matrix.makeRotate(_rotation);

	Vec3d dv = Vec3d(dx * scale, dy * scale, 0) * rotation_matrix;
	
	_center += dv;

	// Action that make sure the rotation center is intersected with the scene
	Vec3d eye, center, up;
	getTransformation(eye, center, up);
	center += (center - eye) * 500;

	osg::ref_ptr<osgUtil::LineSegmentIntersector> ls = new osgUtil::LineSegmentIntersector(eye, center);
	osgUtil::IntersectionVisitor iv(ls);
	if (_view.valid())
	{
		_view->getCamera()->accept(iv);
		auto newCenter = ls->getFirstIntersection().getWorldIntersectPoint();
		_distance = (newCenter - eye).length();

		_center = newCenter;
	}

	return true;
}

void MapController::zoomModel(const float dy, bool pushForwardIfNeeded)
{
	// scale
	float scale = 1.0f + dy;

	// minimum distance
	float minDist = _minimumDistance;
	if (getRelativeFlag(_minimumDistanceFlagIndex))
		minDist *= _modelSize;

	if (_distance*scale > minDist)
	{
		// regular zoom
		_distance *= scale;
	}
	else
	{
		if (pushForwardIfNeeded)
		{
			// push the camera forward
			Vec3d eye, center, up;
			getTransformation(eye, center, up);
			center += (center - eye) * 500;

			osg::ref_ptr<osgUtil::LineSegmentIntersector> ls = new osgUtil::LineSegmentIntersector(eye, center);
			osgUtil::IntersectionVisitor iv(ls);
			if (_view.valid())
			{
				_view->getCamera()->accept(iv);
				auto newCenter = ls->getFirstIntersection().getWorldIntersectPoint();
				_distance += (newCenter - _center).length();
				_center = newCenter;
			}
		}
		else
		{
			// set distance on its minimum value
			_distance = minDist;
		}
	}
}

void MapController::rotateWithFixedVertical(const float dx, const float dy)
{
	CoordinateFrame coordinateFrame = getCoordinateFrame(m_center);
	Vec3d localUp = getUpVector(coordinateFrame);

	// Keep elevation above certain value
	if (getElevation() > -0.0 && dy > 0)
		rotateYawPitch(_rotation, dx, 0, localUp);
	else
		rotateYawPitch(_rotation, dx, dy, localUp);
}

// screensavers
void MapController::screenSaversMovement()
{
	// Rotate around longitude direction
	setHeading(getHeading() + 0.002);
}

void MapController::screenSaving(bool on)
{
	_isTraveling = on;
	_isScreenSaving = on;
}
