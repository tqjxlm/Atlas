#include "PathRoaming.h"

// #include <windows.h>

#include <fstream>
using namespace std;

#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QToolButton>

#include <osg/ShapeDrawable>
#include <osgGA/AnimationPathManipulator>

#include <ViewerWidget/ViewerWidget.h>
#include <MapController/MapController.h>

PathRoaming::PathRoaming()
{
  _pluginName     = tr("Path Roaming");
	_pluginCategory = "Effect";

	endDrawing();
	_DCCount = 0;

	_pathVertex = new osg::Vec3Array;
  _points     = new osg::Vec3Array;

	_allPathNode = new osg::Group;
}

PathRoaming::~PathRoaming()
{
}

void  PathRoaming::setupUi(QToolBar *toolBar, QMenu *menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("showRoamAction"));
	_action->setCheckable(true);
  QIcon  icon10;
	icon10.addFile(QStringLiteral("resources/icons/pathroam.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon10);

	_action->setText(tr("Show Path Roaming"));
	_action->setToolTip(tr("Show Recorded Path Roaming"));

	registerMutexAction(_action);

	roamRecordAction = new QAction(_mainWindow);
	roamRecordAction->setObjectName(QStringLiteral("roamRecordAction"));
	roamRecordAction->setCheckable(true);
  QIcon  icon20;
	icon20.addFile(QStringLiteral("resources/icons/record.png"), QSize(), QIcon::Normal, QIcon::Off);
	roamRecordAction->setIcon(icon20);

	roamRecordAction->setText(tr("Record Roaming Path"));
	roamRecordAction->setToolTip(tr("Start Recordeding a Path for Roaming"));

	roamPlaybackAction = new QAction(_mainWindow);
	roamPlaybackAction->setObjectName(QStringLiteral("roamPlaybackAction"));
	roamPlaybackAction->setCheckable(true);
  QIcon  icon21;
	icon21.addFile(QStringLiteral("resources/icons/playback.png"), QSize(), QIcon::Normal, QIcon::Off);
	roamPlaybackAction->setIcon(icon21);

	roamPlaybackAction->setText(tr("Playback Roaming Path"));
	roamPlaybackAction->setToolTip(tr("Playback Roaming Path"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
	connect(roamRecordAction, SIGNAL(toggled(bool)), this, SLOT(roamRecordSlot(bool)));
	connect(roamPlaybackAction, SIGNAL(triggered()), this, SLOT(roamPlaybackSlot()));

  QToolButton *groupButton = new QToolButton(_mainWindow);
	groupButton->setObjectName("PathRoamGroupButton");
  QIcon  icon1;
	icon1.addFile(QString::fromUtf8("resources/icons/pathroam.png"), QSize(), QIcon::Normal, QIcon::Off);
	groupButton->setIcon(icon1);
	groupButton->setPopupMode(QToolButton::InstantPopup);
  // groupButton->setDefaultAction(_action);
	groupButton->setCheckable(true);
	groupButton->setText(tr("Roaming"));
	groupButton->setToolTip(tr("Path Roaming"));
	QMenu *pathRoamMenu = new QMenu(groupButton);
	pathRoamMenu->setTitle(tr("Path Roaming"));
	pathRoamMenu->setIcon(icon1);
	pathRoamMenu->addAction(_action);
	pathRoamMenu->addAction(roamRecordAction);
	pathRoamMenu->addAction(roamPlaybackAction);
	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
	groupButton->setMenu(pathRoamMenu);

	toolBar->addWidget(groupButton);
	menu->addMenu(pathRoamMenu);
}

void  PathRoaming::onLeftButton()
{
  if (!_isDrawing && (_DCCount == 0))
	{
		_pathGeometry = new osg::Geometry;

		_points->clear();
		_pathVertex->clear();

		_pathVertex->push_back(_anchoredWorldPos);

		_currentDrawNode = new osg::Geode;
		_currentDrawNode->addDrawable(_pathGeometry);
		_currentAnchor->addChild(_currentDrawNode);

		_currentAnchor->addChild(_allPathNode);

		beginDrawing();
	}

	if (_DCCount == 0)
	{
		_points->push_back(_currentWorldPos);
		_pathVertex->push_back(_anchoredWorldPos);

		_allPathNode->addChild(createBox(_anchoredWorldPos));

		_pathGeometry->setVertexArray(_pathVertex);

		_pathGeometry->removePrimitiveSet(0);
		_pathGeometry->addPrimitiveSet(new osg::DrawArrays(osg::DrawArrays::LINE_STRIP, 0, _pathVertex->size()));
		_pathGeometry->dirtyDisplayList();
	}
}

void  PathRoaming::onMouseMove()
{
	if (_isDrawing)
	{
		_pathVertex->pop_back();
		_pathVertex->push_back(_anchoredWorldPos);
		_pathGeometry->setVertexArray(_pathVertex);
		_pathGeometry->dirtyDisplayList();
	}
}

void  PathRoaming::onRightButton()
{
	if (_isDrawing)
	{
		_currentAnchor->removeChild(_currentDrawNode);
		_currentAnchor->removeChild(_allPathNode);
		_allPathNode->removeChildren(0, _allPathNode->getNumChildren());
		_points->clear();
		_pathVertex->clear();
		endDrawing();
	}
}

void  PathRoaming::onDoubleClick()
{
	if (_DCCount % 2 == 0)
	{
		if (_mainViewer->getMainView())
		{
      _manipulator = dynamic_cast<MapController *>
                     (_mainViewer->getMainView()->getCameraManipulator());
			_cameraPos = _manipulator->getInverseMatrix();

			if (_allPathNode->getNumChildren() == 0)
      {
        return;
      }

			if (_allPathNode->getNumChildren() == 1)
			{
        osg::ref_ptr<osgGA::AnimationPathManipulator>  apm = new osgGA::AnimationPathManipulator("path_roam.path");
				_mainViewer->getMainView()->setCameraManipulator(apm);
			}
			else
			{
        osg::ref_ptr<osgGA::AnimationPathManipulator>  apm = new osgGA::AnimationPathManipulator;
				apm->setAnimationPath(createPath());
				_mainViewer->getMainView()->setCameraManipulator(apm);
			}

			endDrawing();
			_DCCount++;
		}
	}
	else
	{
		_mainViewer->getMainView()->setCameraManipulator(_manipulator);
		_mainViewer->getMainView()->getCameraManipulator()->setByInverseMatrix(_cameraPos);

		_currentAnchor->removeChild(_currentDrawNode);
		_currentAnchor->removeChild(_allPathNode);
		_allPathNode->removeChildren(0, _allPathNode->getNumChildren());
		_points->clear();

		_currentDrawNode->removeDrawable(_pathGeometry);
		_pathVertex->clear();
		endDrawing();
		_DCCount--;
	}
}

osg::Geode * PathRoaming::createBox(osg::Vec3 center)
{
	_pathNode = new osg::Geode;
	_pathNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
	center.z() += 1;
  osg::ref_ptr<osg::ShapeDrawable>  sd = new osg::ShapeDrawable(new osg::Box(center, 1, 1, 1));
	sd->setColor(osg::Vec4(1, 0, 0, 1));
	_pathNode->addDrawable(sd);

	return _pathNode.release();
}

float  PathRoaming::getRunTime(osg::Vec3 res, osg::Vec3 des)
{
	return (res - des).length() * 0.1;
}

osg::AnimationPath * PathRoaming::createPath()
{
  osg::ref_ptr<osg::AnimationPath>  anim = new osg::AnimationPath;
	anim->setLoopMode(osg::AnimationPath::LOOP);

  float  time  = 0.0;
  float  angle = 0.0;
  float  roll  = osg::inDegrees(70.0f);

	if (_points.valid())
	{
    for (osg::Vec3Array::iterator iter = _points->begin();; )
		{
      osg::Vec3  pos(*iter);
      iter++;

			if (iter != _points->end())
			{
				if (iter->x() > pos.x())
				{
					angle = osg::inDegrees(90.0f) - atan((iter->y() - pos.y()) / (iter->x() - pos.x()));

					if (angle < 0)
          {
            angle += osg::inDegrees(90.0f);
          }
        }
				else
				{
					angle = -(osg::inDegrees(90.0f) + atan((iter->y() - pos.y()) / (iter->x() - pos.x())));

					if (angle > 0)
          {
            angle = -(osg::inDegrees(90.0f) - angle);
          }
        }

        roll  = osg::inDegrees(90.0f) + atan((iter->z() - pos.z()) / (osg::Vec2(iter->x(), iter->y()) - osg::Vec2(pos.x(), pos.y())).length());
				roll -= osg::inDegrees(30.0f);

				pos.z() += 100;
        osg::Quat  rotate(osg::Quat(roll, osg::Vec3(1.0, 0, 0)) * osg::Quat(-angle, osg::Vec3(0, 0, 1.0)));
				anim->insert(time, osg::AnimationPath::ControlPoint(pos, rotate));
				time += getRunTime(pos, *iter);
			}
			else
			{
				anim->insert(time, osg::AnimationPath::ControlPoint(pos, osg::Quat(osg::Vec4(0.0, 0.0, 0.0, 0.0))));
				break;
			}
		}

    ofstream  out("path_roam.path");
		anim->write(out);
		out.close();
	}

	return anim.release();
}

void  PathRoaming::roamRecordSlot(bool checked)
{
	if (checked)
	{
		keybd_event(90, 0, 0, 0);
		Sleep(100);
		keybd_event(90, 0, KEYEVENTF_KEYUP, 0);
	}
}

void  PathRoaming::roamPlaybackSlot()
{
	keybd_event(88, 0, 0, 0);
	Sleep(100);
	keybd_event(88, 0, KEYEVENTF_KEYUP, 0);
	roamRecordAction->setChecked(false);
}
