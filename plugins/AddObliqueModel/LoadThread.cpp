#include "LoadThread.h"

#include <osg/Transform>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>

LoadThread::LoadThread(QMutex& loadingLock, osg::Group* model, const QFileInfoList& allFileList) 
    : _loadingLock(loadingLock)
    , _model(model)
    , _allFileList(allFileList)
{
}

void LoadThread::run()
{
	_loadingLock.lock();

	int i = 0;
	float pbUnit = qAbs( (100 - 10.0f - 1.0f) / (_allFileList.length() - 1));

	foreach(QFileInfo fileInfo, _allFileList)
	{
		float pbValue = 10 + i * pbUnit;
		i += 1;
		emit progress(pbValue);

		osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(fileInfo.absoluteFilePath().toLocal8Bit().toStdString());
		if (!node.valid())
			continue;
		QString nodestrpath = fileInfo.absoluteFilePath();
		node->setName(nodestrpath.toLocal8Bit().toStdString());
		_model->addChild(node);
	}

	emit progress(99);
	emit done();

	_loadingLock.unlock();
}
