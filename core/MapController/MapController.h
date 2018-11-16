#ifndef MAPCONTROLLER_H
#define MAPCONTROLLER_H

#include "MapController_global.h"

#include <QMap>

#include <QObject>
#include <osgGA/OrbitManipulator>
#include <osgViewer/View>
#include <osg/PositionAttitudeTransform>

namespace osgViewer {
	class View;
}

namespace osg {
	class PositionAttitudeTransform;
}

/** A camera controller that fits projected maps
  * 
  * It defines moving strategies for navigating between coords and nodes
  */
class MAPCONTROLLER_EXPORT MapController :public QObject, public osgGA::OrbitManipulator
{
	Q_OBJECT

public:
	MapController(osg::ref_ptr<osg::Node> dataRoot);

	// Event handler that is called every event traversal
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

	// Update camera that is called every event traversal
	virtual void updateCamera(osg::Camera &camera) override;

	// Set the center indicator of the camera
	void setCenterIndicator(osg::PositionAttitudeTransform* center);

protected:
	// Mouse wheel event defined by OribitManipulator
	virtual bool handleMouseWheel(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

	// Middle button event defined by OribitManipulator
	virtual bool performMovementMiddleMouseButton(const double eventTimeDelta, const double dx, const double dy) override;

	// Zoom function defined by OribitManipulator
	virtual void zoomModel(const float dy, bool pushForwardIfNeeded) override;

	// Rotate function defined by OrbitManipulator
	virtual void rotateWithFixedVertical(const float dx, const float dy) override;

    // Keyboard function defined by OrbitManipulator
    virtual bool handleKeyDown(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

    // Keyboard function defined by OrbitManipulator
    virtual bool handleKeyUp(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override;

    // Action that make sure the rotation center is intersected with the scene
    virtual void stickToScene();

    void updateDriveDirection();

public slots:
	// Begin or stop predefined movement
	void enableAnimation(bool enabled);

	// Begin or stop screen saving movement
	void screenSaving(bool on);

	// Move to the given point with a predefined movement
	void flyToPoint(osg::Vec3 targetPosition, osg::Vec3 targetNormal, float targetDistance = 500.0f);

	// Fit view on node immediately
	void fitViewOnNode(const osg::Node* scene, double addHeight);

	// Fit view on bouding immediately
	void fitViewOnBounding(const osg::BoundingSphere* bs, double addHeight);

protected slots:
	void screenSaversMovement();
	void flyingMovement();
    void drivingMovement();

private:
	// Movement switches
	bool _inAnimation;
	bool _isFlying;
	bool _isScreenSaving;

	// Movement targets
	osg::Vec3 _targetPosition;
	osg::Vec3 _targetNormal;
	float _targetDistance;
	double _targetRoll;

	// Camera position, in world coord
	osg::Vec3d m_eye;
	osg::Vec3d m_center;
	osg::Vec3d m_up;

	// Movement settings
	int _interval;
	double _totalTime;
	int _timeStamp;
	int _totalSteps;
	int _stepCount;

	// Movement status
	osg::Vec3 _acceleration;
	osg::Vec3 _speed;

	osg::ref_ptr<osg::PositionAttitudeTransform> _centerIndicator;
	osg::ref_ptr<osgViewer::View> _view;

    // Driver mode
    bool _isDriving;
    osg::Vec3 _driveDirection;
    osg::Vec3 _front;
    osg::Vec3 _right;
    double _driveSpeed;
    QMap<int, bool> _keyPressed;

    // Data root to stick to
    osg::ref_ptr<osg::Node> _dataRoot;
};

#endif
