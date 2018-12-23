#include "MousePicker.h"

#include <QMap>
#include <QPair>
#include <QLabel>
#include <QStatusBar>

#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osgEarth/Terrain>
#include <osg/PositionAttitudeTransform>
#include <osgSim/OverlayNode>
#include <osgViewer/View>

#include <DataManager/FindNode.hpp>

osgEarth::GeoPoint                                     MousePicker::_currentGeoPos;
osg::Vec3d                                             MousePicker::_currentLocalPos;
osg::Vec3d                                             MousePicker::_currentWorldPos;
osgUtil::LineSegmentIntersector::Intersections         MousePicker::_intersections;
osgUtil::LineSegmentIntersector::Intersection          MousePicker::_nearestIntesection;
osg::ref_ptr<osg::Group>                               MousePicker::_root        = NULL;
osg::ref_ptr<osgSim::OverlayNode>                      MousePicker::_overlayNode = NULL;
osg::ref_ptr<osg::Group>                               MousePicker::_subgraph    = NULL;
osg::ref_ptr<osg::Group>                               MousePicker::_drawRoot    = NULL;
osg::ref_ptr<osg::Group>                               MousePicker::_mapRoot     = NULL;
osg::ref_ptr<osg::Group>                               MousePicker::_dataRoot    = NULL;
osg::ref_ptr<osgEarth::MapNode>                        MousePicker::_mapNode[MAX_SUBVIEW];
osg::ref_ptr<osgEarth::Map>                            MousePicker::_mainMap[MAX_SUBVIEW];
DataManager                                           *MousePicker::_dataManager     = NULL;
SettingsManager                                       *MousePicker::_settingsManager = NULL;
ViewerWidget                                          *MousePicker::_mainViewer      = NULL;
QWidget                                               *MousePicker::_mainWindow      = NULL;
bool                                                   MousePicker::_isValid         = false;
QLabel                                                *MousePicker::_labelWorldCoord;
QLabel                                                *MousePicker::_labelGeoCoord;
osg::ref_ptr<const osgEarth::SpatialReference>         MousePicker::_globalSRS = NULL;
const char                                            *MousePicker::_globalWKT = NULL;
static osg::ref_ptr<const osgEarth::SpatialReference>  srs_wgs84               = osgEarth::SpatialReference::get("wgs84");
static const double                                    DBL_LMT                 = 0.0000001;

MousePicker::MousePicker():
	_activated(true)
{
}

MousePicker::~MousePicker()
{
}

bool  MousePicker::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
	if (_activated == false)
	{
		return false;
	}

	osgViewer::View *view = dynamic_cast<osgViewer::View *>(&aa);

	switch (ea.getEventType())
	{
	case (osgGA::GUIEventAdapter::PUSH):

		if (ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
		{
			emit  changeMouseType(osgViewer::GraphicsWindow::HandCursor);
		}

		if (view) { pick(view, ea); }

		return false;
	case (osgGA::GUIEventAdapter::RELEASE):

		if (ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
		{
			emit  changeMouseType(osgViewer::GraphicsWindow::LeftArrowCursor);
		}

		return false;
	case (osgGA::GUIEventAdapter::MOVE):
	case (osgGA::GUIEventAdapter::DOUBLECLICK):
	case (osgGA::GUIEventAdapter::KEYDOWN):

		if (view) { pick(view, ea); }

		return false;
	default:

		return false;
	}
}

void  MousePicker::pick(osgViewer::View *view, const osgGA::GUIEventAdapter &ea)
{
	getPos(view, ea);

	if (pointValid())
	{
		osgEarth::GeoPoint  latLon;
		_currentGeoPos.transform(srs_wgs84, latLon);
		_labelWorldCoord->setText(tr("Coordinate: [%1, %2, %3]")
		                          .arg(_currentGeoPos.x(), 0, 'f', 2)
		                          .arg(_currentGeoPos.y(), 0, 'f', 2)
		                          .arg(_currentGeoPos.z(), 0, 'f', 2));
		_labelGeoCoord->setText(tr("Lat Lon: [%1, %2, %3]")
		                        .arg(latLon.x(), 0, 'f', 2)
		                        .arg(latLon.y(), 0, 'f', 2)
		                        .arg(latLon.z(), 0, 'f', 2));
	}
	else
	{
		_labelWorldCoord->setText(tr("World Coordinate: NULL"));
		_labelGeoCoord->setText(tr("Geographic Coordinate: NULL"));
	}
}

void  MousePicker::getPos(osgViewer::View              *view,
                          const osgGA::GUIEventAdapter &ea)
{
	_isValid = false;

	// Only interact with data nodes and map nodes
	if (view->computeIntersections(ea, _intersections, INTERSECT_IGNORE))
	{
		for (const auto &intersection : _intersections)
		{
			bool  visible = true;

			for (const auto node : intersection.nodePath)
			{
				// Only count the intersection in main view
				if ((node->getNodeMask() & SHOW_IN_WINDOW_1) == 0)
				{
					visible = false;
					break;
				}
			}

			if (visible)
			{
				_nearestIntesection = intersection;
				_currentLocalPos    = _nearestIntesection.getLocalIntersectPoint();
				_currentWorldPos    = _nearestIntesection.getWorldIntersectPoint();

				// Convert to geographic coordinate, if it should fail, mark this point as invalid
				_isValid = _currentGeoPos.fromWorld(_globalSRS, _currentWorldPos);

				return;
			}
		}
	}
}

void  MousePicker::setupUi(QStatusBar *statusBar)
{
	QSizePolicy  siePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	_labelWorldCoord = new QLabel();
	_labelGeoCoord   = new QLabel();

	_labelWorldCoord->setSizePolicy(siePolicy);
	_labelGeoCoord->setSizePolicy(siePolicy);

	_labelWorldCoord->setFixedSize(400, 20);
	_labelGeoCoord->setFixedSize(400, 20);

	statusBar->addWidget(_labelWorldCoord);
	statusBar->addWidget(_labelGeoCoord);
}

void  MousePicker::registerData(QWidget *mainWindow, DataManager *dataManager, ViewerWidget *mainViewer,
                                osg::Group *root, const osgEarth::SpatialReference *globalSRS)
{
	_mainWindow  = mainWindow;
	_dataManager = dataManager;
	_mainViewer  = mainViewer;
	_globalSRS   = globalSRS;
	_globalWKT   = _globalSRS->getWKT().c_str();
	_root        = root;

	_overlayNode = dynamic_cast<osgSim::OverlayNode *>(findNodeInNode("Data Overlay", _root));
	_drawRoot    = findNodeInNode("Draw Root", _root)->asGroup();
	_mapRoot     = findNodeInNode("Map Root", _root)->asGroup();
	_subgraph    = _overlayNode->getOverlaySubgraph()->asGroup();
	_dataRoot    = findNodeInNode("Data Root", _overlayNode)->asGroup();

	for (unsigned i = 0; i < MAX_SUBVIEW; i++)
	{
		osg::Node *map = findNodeInNode(QString("Map%1").arg(i).toStdString(), _mapRoot);

		if (map)
		{
			_mapNode[i] = dynamic_cast<osgEarth::MapNode *>(map);
			_mainMap[i] = _mapNode[i]->getMap();
		}
		else
		{
			_mapNode[i] = NULL;
		}
	}
}

void  MousePicker::registerSetting(SettingsManager *settingsMenager)
{
	_settingsManager = settingsMenager;
}

bool  MousePicker::pointValid()
{
	return _isValid;
}

void  MousePicker::defaultOperation(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
}
