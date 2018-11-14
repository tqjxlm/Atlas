#include "ModelFlatten.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QIcon>
#include <QAction>
#include <QToolBar>
#include <QMenu>

#include "FlattenVisitor.hpp"

ModelFlatten::ModelFlatten()
{
    _pluginName = tr("Model Flatten");

    _pluginCategory = "Edit";
}

ModelFlatten::~ModelFlatten()
{
    QDir dir;
    for (auto itr = _tempFileList.begin(); itr != _tempFileList.end(); itr++)
    {
        for (auto it = itr->begin(); it != itr->end(); it++)
        {
            dir.remove(*it);
        }
    }
}

void ModelFlatten::setupUi(QToolBar * toolBar, QMenu * menu)
{
    _action = new QAction(_mainWindow);
    _action->setObjectName(QStringLiteral("flattenAction"));
    _action->setCheckable(true);
    _action->setEnabled(true);
    QIcon icon24;
    icon24.addFile(QStringLiteral("resources/icons/bulldozer.png"), QSize(), QIcon::Normal, QIcon::Off);
    _action->setIcon(icon24);
    _action->setVisible(true);

    _action->setText(tr("Flatten"));
    _action->setToolTip(tr("Flatten Model to Polygon"));

    connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));
    registerMutexAction(_action);

    toolBar->addAction(_action);
    menu->addAction(_action);
}

void ModelFlatten::onLeftButton()
{
    if (_isDrawing)
    {
        if (!_activatedModel)
        {
            return;
        }
        _boundary->push_back(osg::Vec2(_currentLocalPos.x(), _currentLocalPos.y()));
    }
    else
    {
        _activatedModel = NULL;
        auto its = _intersections.begin();
        for (auto it = its->nodePath.begin(); it != its->nodePath.end(); ++it)
        {
            if ((*it)->getNumParents() != 0 && (*it)->getParent(0) == _pluginRoot)
            {
                _activatedModel = dynamic_cast<osg::MatrixTransform*>(*it);
            }
        }

        if (!_activatedModel)
        {
            QMessageBox msg(QMessageBox::Warning, "Error", tr("Please operate on a model!"), QMessageBox::Ok);
            msg.setWindowModality(Qt::WindowModal);
            msg.exec();
            DrawSurfacePolygon::onRightButton();
            return;
        }

        _boundary = new osg::Vec2Array;
        _boundary->push_back(osg::Vec2(_currentLocalPos.x(), _currentLocalPos.y()));
        _avgHeight = 0.0;
    }
    _avgHeight += _currentLocalPos.z();

    DrawSurfacePolygon::onLeftButton();
}

void ModelFlatten::onDoubleClick()
{
    if (_isDrawing)
    {
        _avgHeight /= _boundary->size();
        std::vector<osg::ref_ptr<osg::Node>> childToDelete;
        std::vector<osg::ref_ptr<osg::Node>> childToAdd;

        osgUtil::Optimizer::TextureVisitor tv(true, false, false, false, false, false);
        osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options;
        options->setOptionString("OutputTextureFiles WriteImageHint=IncludeData");

        osg::ref_ptr<osgDB::ReaderWriter::Options> read_opt = new osgDB::ReaderWriter::Options;
        read_opt->setOptionString("OutputTextureFiles WriteImageHint=IncludeData");

        QStringList tempList;

        int processValue = _activatedModel->getNumChildren();
        QProgressDialog process;
        process.setLabelText(tr("processing..."));
        process.setRange(0, processValue);
        process.setModal(true);
        process.setWindowModality(Qt::WindowModal);
        process.setCancelButtonText(tr("cancel"));
        process.show();

        for (unsigned int i = 0; i < _activatedModel->getNumChildren(); i++)
        {
            FlattenVisitor fv(tempList, _boundary, _avgHeight);
            osg::PagedLOD* origChild = dynamic_cast<osg::PagedLOD*>(_activatedModel->getChild(i));
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
            process.setValue(i);
            if (process.wasCanceled())
            {
                process.close();

                _currentDrawNode->removeDrawables(0, _currentDrawNode->getNumDrawables());
                endDrawing();

                _tempFileList[_currentDrawNode->getName().c_str()] = tempList;

                return;
            }
        }
        process.close();

        _tempFileList[_currentDrawNode->getName().c_str()] = tempList;

        for (int i = 0; i < childToDelete.size(); i++)
        {
            _activatedModel->removeChild(childToDelete[i]);
            _activatedModel->addChild(childToAdd[i]);
        }

        _currentDrawNode->removeDrawables(0, _currentDrawNode->getNumDrawables());
        endDrawing();

        recordCurrent();
    }
}