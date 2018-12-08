#pragma once

#include <osgUtil/Optimizer>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>

#include <QStringList>
#include <QFileInfo>
#include <QString>

class FlattenVisitor : public osg::NodeVisitor
{
public:
    FlattenVisitor(QStringList& tempList, osg::ref_ptr<osg::Vec2Array> boundary, double height = 0.0)
        : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _boundary(boundary),
        _tempFiles(tempList),
        _childChanged(false)
    {
        _noUnrefVisitor = new osgUtil::Optimizer::TextureVisitor(true, false, false, false, false, false);
        _autoUnrefVisitor = new osgUtil::Optimizer::TextureVisitor(true, true, false, false, false, false);

        _planeVector.set(0, 0, 1, -height);
    }

    FlattenVisitor(QStringList& tempList, osg::ref_ptr<osg::Vec2Array> boundary, const osg::Plane& refPlane)
        : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _boundary(boundary),
        _tempFiles(tempList),
        _childChanged(false)
    {
        _noUnrefVisitor = new osgUtil::Optimizer::TextureVisitor(true, false, false, false, false, false);
        _autoUnrefVisitor = new osgUtil::Optimizer::TextureVisitor(true, true, false, false, false, false);

        _planeVector = refPlane.asVec4();
    }

    ~FlattenVisitor()
    {
    }

    void apply(osg::Geode& geode)
    {
        for (unsigned int i = 0; i < geode.getNumDrawables(); i++)
        {
            osg::Geometry* geom = geode.getDrawable(i)->asGeometry();
            if (geom)
            {
                osg::Vec3Array* vertex = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
                if (!vertex)
                    continue;
                for (int i = 0; i < vertex->size(); i++)
                {
                    if (pointInPolygon((*vertex)[i]))
                    {
                        (*vertex)[i].z() = -(_planeVector.w() + _planeVector.x() * (*vertex)[i].x() + _planeVector.y() * (*vertex)[i].y()) / _planeVector.z();
                        _affected = true;
                    }
                }
            }
        }

        if (_affected && !geode.getName().empty())
        {
            QString saveFileName = QFileInfo(geode.getName().c_str()).fileName().split('.').first() + "~.osgb";
            geode.accept(*_autoUnrefVisitor);
            std::string writePath(_currentDir + "/" + saveFileName.toLocal8Bit().toStdString());
            geode.setName(writePath);
            osgDB::writeNodeFile(geode, writePath, _opt);
            _tempFiles.append(writePath.c_str());
            _newFileName = saveFileName;
            _childChanged = true;
        }
    }

    void apply(osg::PagedLOD& plod)
    {
        _affected = false;
        for (unsigned int i = 0; i < plod.getNumChildren(); i++)
        {
            plod.getChild(i)->accept(*this);
        }

        if (_affected)
        {
            for (unsigned int i = 0; i < plod.getNumFileNames(); i++)
            {
                _childChanged = false;
                std::string nextPath = _currentDir + "/" + plod.getFileName(i);
                osg::ref_ptr<osg::Node> nextLevel = osgDB::readNodeFile(nextPath);
                if (!nextLevel.valid())
                    continue;
                nextLevel->setName(nextPath);
                nextLevel->accept(*_noUnrefVisitor);
                nextLevel->accept(*this);
                if (_childChanged)
                {
                    plod.setFileName(i, _newFileName.toLocal8Bit().toStdString());
                }
            }

            if (!plod.getName().empty())
            {
                QString saveFileName = QFileInfo(plod.getName().c_str()).fileName().split('.').first() + "~.osgb";
                plod.accept(*_autoUnrefVisitor);
                std::string writePath(_currentDir + "/" + saveFileName.toLocal8Bit().toStdString());
                plod.setName(writePath);
                osgDB::writeNodeFile(plod, writePath, _opt);
                _tempFiles.append(writePath.c_str());
                _newFileName = saveFileName;
            }

            _childChanged = true;
        }
    }

    void apply(osg::Group& group)
    {
        bool changed = false;
        for (unsigned int i = 0; i < group.getNumChildren(); i++)
        {
            _childChanged = false;
            group.getChild(i)->accept(*this);
            if (_childChanged)
            {
                changed = true;
            }
        }

        if (changed && !group.getName().empty())
        {
            QString saveFileName = QFileInfo(group.getName().c_str()).fileName().split('.').first() + "~.osgb";
            group.accept(*_autoUnrefVisitor);
            std::string writePath(_currentDir + "/" + saveFileName.toLocal8Bit().toStdString());
            group.setName(writePath);
            osgDB::writeNodeFile(group, writePath, _opt);
            _tempFiles.append(writePath.c_str());
            _newFileName = saveFileName;
            _childChanged = true;
        }
    }

    bool pointInPolygon(osg::Vec3& point) {
        int i, j, nvert = _boundary->size();
        bool c = false;

        for (i = 0, j = nvert - 1; i < nvert; j = i++) {
            if (((_boundary->at(i).y() >= point.y()) != (_boundary->at(j).y() >= point.y())) &&
                (point.x() <= (_boundary->at(j).x() - _boundary->at(i).x()) * (point.y() - _boundary->at(i).y()) / (_boundary->at(j).y() - _boundary->at(i).y()) + _boundary->at(i).x())
                )
                c = !c;
        }
        return c;
    }

    bool isChanged()
    {
        return _childChanged;
    }

    void setDir(const std::string& dir)
    {
        _currentDir = dir;
    }

    void setOption(osgDB::ReaderWriter::Options* option)
    {
        _opt = option;
    }

protected:
    osg::ref_ptr<osg::Vec2Array> _boundary;
    bool _childChanged;
    bool _affected;
    std::string _currentDir;
    QString _newFileName;
    osg::Vec4 _planeVector;
    osg::ref_ptr<osgUtil::Optimizer::TextureVisitor> _noUnrefVisitor;
    osg::ref_ptr<osgUtil::Optimizer::TextureVisitor> _autoUnrefVisitor;
    osgDB::ReaderWriter::Options* _opt;
    QStringList& _tempFiles;
};
