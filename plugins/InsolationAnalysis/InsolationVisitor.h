#pragma once

#include <QThread>

#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/Geode>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>

//获取坐标转换matrix
class InsolationVisitor : public osg::NodeVisitor
{
public:
	InsolationVisitor() :osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
	{}

	~InsolationVisitor()
	{}

	virtual void apply(osg::Geode& geode)
	{
		//计算当前geode节点对应的世界变换矩阵，用来计算geode中顶点对应的世界坐标
		_geodeMatrix = osg::computeLocalToWorld(getNodePath());

		traverse(geode);
	}

	osg::Matrix getGeodeMatrix() { return _geodeMatrix; }

private:
	osg::Matrix _geodeMatrix;//本地转世界坐标matrix
};

//用于精细层
class FinestInsolationVisitor : public osg::NodeVisitor
{
public:
	FinestInsolationVisitor() :osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
	{
		_geodeGroup = new osg::Group;
	}

	~FinestInsolationVisitor()
	{}

	void apply(osg::PagedLOD& plod)
	{
		//loop+next：获取最精细层pagedLOD（判断file_name_list）
		for (unsigned int i = 0; i < plod.getNumFileNames(); i++)
		{
			std::string nextPath = plod.getDatabasePath() + plod.getFileName(i);
			osg::Node* nextLevel = osgDB::readNodeFile(nextPath);
			if (nextLevel==NULL)
				continue;
			else
			{
				osg::ref_ptr<osg::Geode> tmpGeode = dynamic_cast<osg::Geode*>(nextLevel);
				if (tmpGeode != NULL)//判断是否为finest geode
				{
					tmpGeode = nextLevel->clone(osg::CopyOp::DEEP_COPY_ALL)->asNode()->asGeode();
					//_geodeGroup->addChild(&geode);
					_geodeGroup->addChild(tmpGeode);
				}
				else
					nextLevel->accept(*this);//若是pagedlod则递归
			}
		}
	}

	//group中或包含pagedLOD以及Geode（finest）
	void apply(osg::Group& group)
	{
		for (unsigned int i = 0; i < group.getNumChildren(); i++)
		{
			osg::ref_ptr<osg::Geode> tmpGeode = dynamic_cast<osg::Geode*>(group.getChild(i));
			if (tmpGeode != NULL)//判断是否为finest geode
			{
				tmpGeode = group.getChild(i)->clone(osg::CopyOp::DEEP_COPY_ALL)->asNode()->asGeode();
				//_geodeGroup->addChild(&geode);
				_geodeGroup->addChild(tmpGeode);
			}
			else
			{
				group.getChild(i)->accept(*this);//若是pagedlod则递归
			}
		}
	}

	osg::ref_ptr<osg::Group> getGeodeGroup() { return _geodeGroup; }

private:
	osg::ref_ptr<osg::Group> _geodeGroup;//日照用geode全集
};

class LoadingThread : public QThread
{
	Q_OBJECT

public:
	LoadingThread(osg::ref_ptr<osg::Group> model)
	{
		_isFinished = false;
		_modelGroup = model;
	}
	
	virtual void run() { _modelGroup->accept(_fsv); _isFinished = true; }

	FinestInsolationVisitor getFSV() { return _fsv; }

	bool getFinishStatus() { return _isFinished; }

	void done() { this->terminate(); this->wait(); }

private:
	FinestInsolationVisitor _fsv;
	osg::ref_ptr<osg::Group> _modelGroup;
	bool _isFinished;
};