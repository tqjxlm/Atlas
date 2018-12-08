#pragma once

#include "FlattenVisitor.hpp"

#include <QObject>
#include <QStringList>
#include <QString>
#include <vector>

class FlattenProcess : public QObject
{
  Q_OBJECT

    public slots:
  void doFlatten(osg::Group* selectedNode, QStringList tempList, osg::Vec2Array* boundary, double avgHeight)
  {
    std::vector<osg::ref_ptr<osg::Node>> childToDelete;
    std::vector<osg::ref_ptr<osg::Node>> childToAdd;

    osgUtil::Optimizer::TextureVisitor tv(true, false, false, false, false, false);
    osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options;
    options->setOptionString("OutputTextureFiles WriteImageHint=IncludeData");
    FlattenVisitor fv(tempList, boundary, avgHeight);

    for (unsigned int i = 0; i < selectedNode->getNumChildren(); i++)
    {
      if (_shouldCancel)
      {
        emit finished();
        return;
      }

      osg::PagedLOD* origChild = dynamic_cast<osg::PagedLOD*>(selectedNode->getChild(i));
      if (!origChild)
        continue;
      osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(origChild->getName());
      fv.setDir(origChild->getDatabasePath());
      fv.setOption(options);
      node->setName(origChild->getName());
      node->accept(tv);
      node->accept(fv);
      if (fv.isChanged())
      {
        childToDelete.push_back(origChild);
        childToAdd.push_back(node);
      }
      emit updateProgress(i);
    }

    for (int i = 0; i < childToDelete.size(); i++)
    {
      selectedNode->removeChild(childToDelete[i]);
      selectedNode->addChild(childToAdd[i]);
    }
    emit finished();
  }

  void cancel()
  {
    _shouldCancel = true;
  }

signals:
  void updateProgress(int i);
  void finished();

private:
  bool _shouldCancel = false;

};