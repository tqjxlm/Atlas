#include "ModelFlatten.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressDialog>
#include <QIcon>
#include <QAction>
#include <QToolBar>
#include <QMenu>
#include <QTreeWidgetItem>
#include <QThread>

#include <DataManager/DataRecord.h>

#include "FlattenProcess.h"

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
}

void ModelFlatten::loadContextMenu(QMenu * contextMenu, QTreeWidgetItem * selectedItem)
{
  if (selectedItem->parent()->text(0) == tr("Oblique Imagery Model"))
  {
    auto dataRecord = dynamic_cast<DataRecord*>(selectedItem);
    if (dataRecord && !dataRecord->isLayer() && dataRecord->node())
    {
      contextMenu->addAction(_action);
      _selectedNode = dataRecord->node()->asGroup();
    }
  }
}

void ModelFlatten::onLeftButton()
{
  if (!_isDrawing)
  {
    if (!_selectedNode)
    {
      QMessageBox msg(QMessageBox::Warning, "Error", tr("Please operate on a model!"), QMessageBox::Ok);
      msg.setWindowModality(Qt::WindowModal);
      msg.exec();
      DrawSurfacePolygon::onRightButton();
      return;
    }

    _boundary = new osg::Vec2Array;
    _avgHeight = 0.0;
  }
  _avgHeight += _currentLocalPos.z();
  _boundary->push_back(osg::Vec2(_currentLocalPos.x(), _currentLocalPos.y()));

  DrawSurfacePolygon::onLeftButton();
}

void ModelFlatten::onDoubleClick()
{
  if (_isDrawing)
  {
    // Prepare parameters for use in platten process
    _avgHeight /= _boundary->size();
    QStringList tempList;

    // Show a process dialog
    int processValue = _selectedNode->getNumChildren();
    QProgressDialog* pDialog = new QProgressDialog;
    pDialog->setLabelText(tr("Processing..."));
    pDialog->setRange(0, processValue);
    pDialog->setCancelButtonText(tr("Cancel"));
    pDialog->setWindowTitle(tr("Flatten Model"));
    //process->setAttribute(Qt::WA_DeleteOnClose, true);

    // Run the process in a second thread
    QThread* workerThread = new QThread;
    FlattenProcess* flattenProcess = new FlattenProcess;
    flattenProcess->moveToThread(workerThread);

    connect(this, &ModelFlatten::startProcess, flattenProcess, &FlattenProcess::doFlatten);
    connect(pDialog, &QProgressDialog::canceled, flattenProcess, &FlattenProcess::cancel);
    connect(workerThread, &QThread::finished, flattenProcess, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, pDialog, &QProgressDialog::close);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    connect(flattenProcess, &FlattenProcess::updateProgress, pDialog, &QProgressDialog::setValue);
    connect(flattenProcess, &FlattenProcess::finished, [=]() {
      // Clean up
      _tempFileList[_selectedNode->getName().c_str()] = tempList;
      workerThread->quit();
    });

    // Reset actions
    DrawSurfacePolygon::onRightButton();
    _action->toggle();

    // Start it
    workerThread->start();
    emit startProcess(_selectedNode, tempList, _boundary, _avgHeight);
    pDialog->show();
  }
}