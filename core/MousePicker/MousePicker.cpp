#include "MousePicker.h"

#include <QMap>
#include <QPair>

#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osg/PositionAttitudeTransform>
#include <osgSim/OverlayNode>
#include <osgViewer/View>

#include <DataManager/FindNode.hpp>

osg::Vec3 MousePicker::_currentLocalPos;
osg::Vec3 MousePicker::_currentWorldPos;
osgUtil::LineSegmentIntersector::Intersections MousePicker::_intersections;

osg::ref_ptr<osgSim::OverlayNode> MousePicker::_overlayNode = NULL;

osg::ref_ptr<osg::Group> MousePicker::_root = NULL;
osg::ref_ptr<osg::PositionAttitudeTransform> MousePicker::_subgraph = NULL;
osg::ref_ptr<osg::PositionAttitudeTransform> MousePicker::_drawRoot = NULL;
osg::ref_ptr<osg::PositionAttitudeTransform> MousePicker::_dataRoot = NULL;
osg::ref_ptr<osgEarth::MapNode> MousePicker::_mapNode[MAX_SUBVIEW];
osg::ref_ptr<osgEarth::Map> MousePicker::_mainMap[MAX_SUBVIEW];

DataManager* MousePicker::_dataManager = NULL;
SettingsManager* MousePicker::_settingsManager = NULL;
ViewerWidget* MousePicker::_mainViewer = NULL;
QWidget* MousePicker::_mainWindow = NULL;

const char* MousePicker::_globalWKT = NULL;

bool MousePicker::_isValid = false;

osg::ref_ptr<const osgEarth::SpatialReference> MousePicker::_globalSRS = NULL;

static osg::ref_ptr<const osgEarth::SpatialReference> srs_wgs84 = osgEarth::SpatialReference::get("wgs84");

static const double DBL_LMT = 0.0000001;

MousePicker::MousePicker()
	: _activated(true)
{
}

MousePicker::~MousePicker()
{
}

bool MousePicker::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
	if (_activated == false)
		return false;

	osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
	switch (ea.getEventType())
	{
	case(osgGA::GUIEventAdapter::PUSH):
		if (ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
			emit changeMouseType(osgViewer::GraphicsWindow::HandCursor);
		if (view) pick(view, ea);
		return false;
	case (osgGA::GUIEventAdapter::RELEASE):
		if (ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
			emit changeMouseType(osgViewer::GraphicsWindow::LeftArrowCursor);
		return false;
	case(osgGA::GUIEventAdapter::MOVE):
	case(osgGA::GUIEventAdapter::DOUBLECLICK):
	case(osgGA::GUIEventAdapter::KEYDOWN):
		if (view) pick(view, ea);
		return false;
	default:
		return false;
	}
}

void MousePicker::pick(osgViewer::View* view, const osgGA::GUIEventAdapter& ea)
{
	getPos(view, ea);
	if (pointValid())
	{
		// Update coordinates to the main Ui
		osgEarth::GeoPoint currentGeoPos = osgEarth::GeoPoint(_globalSRS, _currentWorldPos).transform(srs_wgs84);

		emit updateText1(tr("Local Coordinate: [%1, %2, %3]")
			.arg(_currentLocalPos.x(), 0, 'f', 2)
			.arg(_currentLocalPos.y(), 0, 'f', 2)
			.arg(_currentLocalPos.z(), 0, 'f', 2));
		emit updateText2(tr("Projected Coordinate: [%1, %2, %3]")
			.arg(_currentWorldPos.x(), 0, 'f', 2)
			.arg(_currentWorldPos.y(), 0, 'f', 2)
			.arg(_currentWorldPos.z(), 0, 'f', 2));
		emit updateText3(tr("Geographic Coordinate: [%1, %2, %3]")
			.arg(currentGeoPos.x(), 0, 'f', 2)
			.arg(currentGeoPos.y(), 0, 'f', 2)
			.arg(currentGeoPos.z(), 0, 'f', 2));
	}
	else
	{
		emit updateText1(tr("Local Coordinate: NULL"));
		emit updateText2(tr("Projected Coordinate: NULL"));
		emit updateText3(tr("Geographic Coordinate: NULL"));
	}
}

void MousePicker::getPos(osgViewer::View* view,
	const osgGA::GUIEventAdapter& ea)
{
	_isValid = false;

	// Only interact with nodes under _overlayNode
	if (view->computeIntersections(ea, _intersections, view->getCamera()->getCullMask()))
	{
		for (auto intersection : _intersections)
		{
			for (auto parent : intersection.nodePath)
			{
				if (parent == _overlayNode)
				{
					_currentLocalPos = intersection.getLocalIntersectPoint();
					_currentWorldPos = intersection.getWorldIntersectPoint();
					_isValid = true;
					return;
				}
			}
		}
	}
}

void MousePicker::registerData(
	QWidget* mainWindow, DataManager* dataManager, ViewerWidget* mainViewer, 
	osg::Group* root, const osgEarth::SpatialReference* globalSRS)
{
	_mainWindow = mainWindow;
	_dataManager = dataManager;
	_mainViewer = mainViewer;
	_globalSRS = globalSRS;
	_globalWKT = _globalSRS->getWKT().c_str();
	_root = root;

	_overlayNode = dynamic_cast<osgSim::OverlayNode*>(findNodeInNode("World Overlay", _root));
	_drawRoot = findNodeInNode("Draw Root", _root)->asTransform()->asPositionAttitudeTransform();
	_subgraph = _overlayNode->getOverlaySubgraph()->asTransform()->asPositionAttitudeTransform();
	_dataRoot = findNodeInNode("Data Root", _overlayNode)->asTransform()->asPositionAttitudeTransform();

	for (unsigned i = 0; i < MAX_SUBVIEW; i++)
	{
		osg::Node* map = findNodeInNode(QString("Map%1").arg(i).toStdString(), _overlayNode);
		if (map)
		{
			_mapNode[i] = dynamic_cast<osgEarth::MapNode*>(map);
			_mainMap[i] = _mapNode[i]->getMap();
		}
		else
			_mapNode[i] = NULL;
	}
}

void MousePicker::registerSetting(SettingsManager* settingsMenager)
{
	_settingsManager = settingsMenager;
}

bool MousePicker::pointValid()
{
	return _isValid;
}

void MousePicker::defaultOperation(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
}
