#include "ModelPath.h"

#include <QMessageBox>
#include <QDateTime>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QObject>

#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/AnimationPath>
#include <osgDB/ReadFile>

ModelPath::ModelPath()
{
	_pluginName = tr("Model Path");
	_pluginCategory = "Effect";
}

ModelPath::~ModelPath()
{
}

void ModelPath::onLeftButton()
{
	if (!_isDrawing)
	{
		beginDrawing();

		_animGroup = new osg::Group;
		_currentAnchor->addChild(_animGroup);

		_startPoint = _anchoredWorldPos;
		_lastPoint = _startPoint;
		_tmpVec3 = _anchoredWorldPos;

		_currentDrawNode = new osg::Geode();
		_currentDrawNode->setUserValue("distance", 0.0f);
		_animGroup->addChild(_currentDrawNode);

		_lineNode = new osg::Geode();
		_animGroup->addChild(_lineNode);

		_slicerPointList = new osg::Vec3Array;
	}
	else
	{
		if (_lastPoint != _endPoint)
		{
			float distance;
			_currentDrawNode->getUserValue("distance", distance);
			osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>
				(_currentDrawNode->getDrawable(_currentDrawNode->getNumDrawables() - 1)->asGeometry()->getVertexArray());
			for (int i = 0; i < vertices->size() - 1; i++)
			{
				distance += (vertices->at(i) - vertices->at(i + 1)).length();
			}
			_currentDrawNode->setUserValue("distance", distance);
		}

		osg::Vec3Array* lastPoints = (osg::Vec3Array*)(_lineGeom->getVertexArray());
		_slicerPointList->insert(_slicerPointList->end(), lastPoints->begin(), lastPoints->end());

		_lastPoint = _anchoredWorldPos;
	}

	_lineGeom = newLine();
	_lineNode->addDrawable(_lineGeom.get());
	_slicer.setStartPoint(_lastPoint);

	_currentDrawNode->addDrawable(createPointGeode(_lastPoint, _intersections.begin()->getLocalIntersectNormal()));
}

void ModelPath::onRightButton()
{
	if (_isDrawing)
	{
		endDrawing();
		_animGroup->removeChild(_currentDrawNode);
		_animGroup->removeChild(_lineNode);
	}
}

void ModelPath::onDoubleClick()
{
	if (_isDrawing)
	{
		endDrawing();
		_currentDrawNode->addDrawable(createPointGeode(_endPoint, _intersections.begin()->getLocalIntersectNormal()));

		osg::ref_ptr<osg::Vec3Array> tmpFilterPointList = new osg::Vec3Array;
		_pathPointList = new osg::Vec3Array;

		if (_slicerPointList->size() < 2)
		{
			return;
		}
		else
		{
			tmpFilterPointList = DrawSurfaceLine::lineSmoothing(_slicerPointList);

			if (tmpFilterPointList->size() < 2)
			{
				return;
			}
			else
			{
				for (osg::Vec3Array::iterator iter = tmpFilterPointList->begin(); ;)
				{
					osg::Vec3 pos(*iter);
					iter++;
			
					float tmpLength =/* (pos - _tmpVec3).length();*/sqrt((pos.x() - _tmpVec3.x())*(pos.x() - _tmpVec3.x()) + 
						(pos.y() - _tmpVec3.y())*(pos.y() - _tmpVec3.y()) + 
						(pos.z() - _tmpVec3.z())*(pos.z() - _tmpVec3.z()));

					if (iter != tmpFilterPointList->end())
					{
						if (tmpLength > 10 && tmpLength < 30)
						{
							_tmpVec3 = pos;
							_pathPointList->push_back(pos);
						}
						else
							continue;
					}
					else
						break;
				}
			}

			if (_pathPointList->size() < 2)
			{
				QMessageBox msgBox(QMessageBox::Warning,"Notice","too short path",QMessageBox::Cancel,NULL);
				msgBox.setWindowModality(Qt::WindowModal);
				msgBox.exec();
				_animGroup->removeChild(_lineNode);
				_animGroup->removeChild(_currentDrawNode);
				return;
			}

			recordCurrent();

		_animpath = CreateAnimationPath(_pathPointList);
		_animGroup->addChild(CreateVehicleModel(_animpath));
	}
}
}

void ModelPath::recordCurrent()
{
	QString strcurrntime = QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"); 
	QString nodeName = tr("%1: %2").arg(_pluginName).arg(strcurrntime);
	PluginInterface::recordNode(_animGroup,nodeName);
}

void ModelPath::onMouseMove()
{
	if(_isDrawing)
	{
		_endPoint = _anchoredWorldPos;
		if (_lastPoint != _endPoint)
		{
			_slicer.setEndPoint(_endPoint);
			updateIntersectedLine();
		}
	}
}

osg::ref_ptr<osg::Geometry> ModelPath::newLine()
{
	osg::ref_ptr<osg::Geometry> lineGeom = new osg::Geometry;
	lineGeom->setName("Model path");

	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(_style.lineColor);
	lineGeom->setColorArray(color.get(), osg::Array::BIND_OVERALL);

	//lineGeom->getOrCreateStateSet()->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);

	return lineGeom;
}

void ModelPath::setupUi(QToolBar * toolBar, QMenu * menu)
{
	_action = new QAction(_mainWindow);
	_action->setObjectName(QStringLiteral("animpathVehicleAction"));
	_action->setCheckable(true);
	QIcon icon29;
	icon29.addFile(QStringLiteral("resources/icons/car.png"), QSize(), QIcon::Normal, QIcon::Off);
	_action->setIcon(icon29);
	_action->setText(tr("Truck"));

	connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

	registerMutexAction(_action);

	QToolButton *groupButton = toolBar->findChild<QToolButton*>("PathAnimationGroupButton", Qt::FindDirectChildrenOnly);
	if (!groupButton)
	{
		groupButton = new QToolButton(_mainWindow);
		groupButton->setObjectName("PathAnimationGroupButton");
		QIcon amiicon;
		amiicon.addFile(QString::fromUtf8("resources/icons/car.png"), QSize(), QIcon::Normal, QIcon::Off);
		groupButton->setIcon(amiicon);
		groupButton->setText(tr("Path"));
		groupButton->setToolTip(tr("Path animation"));
		groupButton->setPopupMode(QToolButton::InstantPopup);
		QMenu *animiMenu = new QMenu(groupButton);
		animiMenu->addAction(_action);
		animiMenu->setIcon(amiicon);
		animiMenu->setTitle(tr("Path Animation"));
		groupButton->setMenu(animiMenu);
		groupButton->setToolTip(tr("Path Animation"));
		toolBar->addWidget(groupButton);

		menu->addMenu(animiMenu);
	}
	else
	{
		groupButton->menu()->addAction(_action);
	}
	connect(_action, SIGNAL(triggered(bool)), groupButton, SLOT(setChecked(bool)));
}

void ModelPath::updateIntersectedLine()
{
	_slicer.computeIntersections(_dataRoot.get());
	osgSim::ElevationSlice::Vec3dList pointList = _slicer.getIntersections();

	if (!pointList.empty())
	{
		_lineVertex = new osg::Vec3Array(pointList.begin(), pointList.end());

		_lineGeom->setVertexArray(_lineVertex.get());

		_lineGeom->removePrimitiveSet(0);
		_lineGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, pointList.size()));
		_lineGeom->dirtyDisplayList();
	}
}


osg::ref_ptr<osg::AnimationPath> ModelPath::CreateAnimationPath(osg::ref_ptr<osg::Vec3Array> pointarr)
{
	osg::ref_ptr<osg::AnimationPath> anim=new osg::AnimationPath; 
	anim->setLoopMode(osg::AnimationPath::LOOP);  

	float time=0.0;  
	float angle=0.0;  

	if(pointarr.valid())
	{  
		for(osg::Vec3Array::iterator iter=pointarr->begin(); ;)
		{
			osg::Vec3 pos(*iter);
			pos.z() += 6; 

			iter++;	

			if(iter!=pointarr->end())
			{
				if(iter->x() > pos.x())  
				{  
					angle = osg::inDegrees(90.0f) - atan( (iter->y()-pos.y()) / (iter->x()-pos.x()) );  
					if(angle < 0)  
						angle += osg::inDegrees(90.0f); 
				}
				else
				{
					angle = -(osg::inDegrees(90.0f) + atan( (iter->y()-pos.y()) / (iter->x()-pos.x()) ));  
					if(angle > 0)  
						angle = -(osg::inDegrees(90.0f)-angle);  
				}
				osg::Quat rotate ( osg::Quat(-angle,osg::Vec3(0,0,1.0)) ); 
				anim->insert(time,osg::AnimationPath::ControlPoint(pos,rotate));  
				time+=getRunTime(pos, *iter);  
			} 
			else
			{  
				anim->insert(time,osg::AnimationPath::ControlPoint(pos,osg::Quat(osg::Vec4(0.0,0.0,0.0,0.0))));
				break;  
			}  
		}
	}
	else
		return NULL;

	//ofstream out("Resources/vehicle.path");
	//anim->write(out);
	//out.close();
	return anim;
}

float ModelPath::getRunTime(osg::Vec3 res,osg::Vec3 des)
{  
	float xx=(res.x()-des.x())*(res.x()-des.x());  
	float yy=(res.y()-des.y())*(res.y()-des.y());  
	float zz=(res.z()-des.z())*(res.z()-des.z());  

	float distant=sqrt(xx+yy+zz);  
	return distant/10.0;  
}

osg::ref_ptr<osg::PositionAttitudeTransform> ModelPath::CreateVehicleModel(osg::ref_ptr<osg::AnimationPath> path)
{
	std::string nodepath="Resources/dumptruck.osg";
	osg::ref_ptr<osg::Node> vehicle = osgDB::readNodeFile(nodepath);
	const osg::BoundingSphere& bs = vehicle->getBound();

	osg::ref_ptr<osg::MatrixTransform> mtx = new osg::MatrixTransform();
	mtx->setMatrix(osg::Matrix::rotate(osg::inDegrees(90.0f), osg::Vec3(0.0f,0.0f,1.0f)));

	mtx->addChild(vehicle);

	osg::ref_ptr<osg::PositionAttitudeTransform> pat = new osg::PositionAttitudeTransform();
	pat->setUpdateCallback(new osg::AnimationPathCallback(path,0.0,1.0));
	pat->addChild(mtx);
	pat->setUserValue("nodepath",nodepath);
	return pat;
}
