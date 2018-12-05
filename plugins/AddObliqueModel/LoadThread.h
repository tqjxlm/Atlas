#ifndef LOADTHREAD_H
#define LOADTHREAD_H

#include <QThread>
#include <QMutex>
#include <QFileInfoList>
#include <QString>

#include <osg/BoundingBox>

namespace osg {
	class Group;
	class Node;
}

class LoadThread : public QThread
{
	Q_OBJECT

public:
	LoadThread(QMutex& loadingLock, osg::Group* model, const QFileInfoList& allFileList);

	void run();

signals:
	void progress(int);
	void done();

private:
	QFileInfoList _allFileList;
	osg::Group* _model;
	QMutex& _loadingLock;
};

#endif // LOADTHREAD_H
