#include "AddObliqueModel.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QDirIterator>
#include <QDomDocument>

#include <osg/PositionAttitudeTransform>
#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>
#include <osgEarthDrivers/model_simple/SimpleModelOptions>
#include <osgEarth/ModelLayer>
#include <osgEarthAnnotation/ModelNode>

#include "LoadThread.h"

static const double zOffset = 0.1;

static inline void  getFolderFile(QString &path, QFileInfoList &file_to_use)
{
  QDirIterator  firstLevelDir(path, QDir::Dirs);

	while (firstLevelDir.hasNext())
	{
		firstLevelDir.next();

		if (firstLevelDir.filePath().endsWith('.'))
    {
      continue;
    }

    QDirIterator  secondLevelDir(firstLevelDir.filePath(), QDir::Dirs);

		while (secondLevelDir.hasNext())
		{
			secondLevelDir.next();

			if (secondLevelDir.filePath().endsWith('.'))
      {
        continue;
      }

			file_to_use.push_back(QFileInfo(secondLevelDir.filePath() + "/" + secondLevelDir.fileName() + ".osgb"));
		}
	}
}

AddObliqueModel::AddObliqueModel()
{
  _pluginCategory = "Data";
  _pluginName     = tr("Oblique Imagery Model");
}

AddObliqueModel::~AddObliqueModel()
{
}

void  AddObliqueModel::setupUi(QToolBar *toolBar, QMenu *menu)
{
	// Oblique Photography
  QAction *openOpAction = new QAction(_mainWindow);

	openOpAction->setObjectName(QStringLiteral("openOpAction"));
  QIcon  icon2;
	icon2.addFile(QStringLiteral("resources/icons/oblique_model.png"), QSize(), QIcon::Normal, QIcon::Off);
	openOpAction->setIcon(icon2);
	openOpAction->setText(tr("Oblique"));
	openOpAction->setToolTip(tr("Load an Oblique Imagery model"));

	menu->addAction(openOpAction);
	toolBar->addAction(openOpAction);

	connect(openOpAction, SIGNAL(triggered()), this, SLOT(addObliqueModel()));
}

void AddObliqueModel::onLoadingDone(const QString& nodeName, osg::Node *model, const osgEarth::GeoPoint &geoOrigin)
{
  //osgEarth::Drivers::SimpleModelOptions opt;
  //opt.node() = model;
  //opt.paged() = true;
  //opt.location() = geoOrigin.vec3d() + osg::Vec3d(0, 0, zOffset);

  //osg::ref_ptr<osgEarth::ModelLayer> layer = new osgEarth::ModelLayer(opt);
  //addLayerToMap(nodeName, layer, MODEL_LAYER, _pluginName);

  osgEarth::Viewpoint vp;
  //vp.setNode(layer->getNode());
  vp.setNode(model);
  emit setViewPoint(vp);
  emit loadingDone();

  auto anchorPoint = getNearestAnchorPoint(geoOrigin.vec3d());
  anchorPoint->addChild(model);

  recordNode(model, nodeName);
}

void  AddObliqueModel::loadObliqueModel(const QString& pathXML)
{
  // Check data validity
  QFile  file(pathXML);

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::information(NULL, tr("Error"), tr("Fail to open!"));

		return;
	}

  QDomDocument  doc;

	if (!doc.setContent(&file))
	{
		QMessageBox::information(NULL, tr("Error"), tr("Fail to open!"));
		file.close();

		return;
	}

	if (doc.isNull())
	{
		QMessageBox::information(NULL, tr("Error"), tr("Invalid XML file!"));
		file.close();

		return;
	}

  QFileInfo  fi      = QFileInfo(pathXML);
  QString    XMLPath = fi.absolutePath();

  // Get SRS info
  QString       srsInfo;
  QStringList   _srsOriginInfo;
  QDomElement   xmlroot = doc.documentElement();
  QDomNodeList  list    = xmlroot.childNodes();
  QDomElement   element;

	for (int i = 0; i < list.count(); i++)
	{
		QCoreApplication::processEvents();

		element = list.item(i).toElement();

		if (element.tagName() == "SRS")
    {
      srsInfo = element.text();
    }

    if (element.tagName() == "SRSOrigin")
    {
      _srsOriginInfo = element.text().split(',');
    }
  }

	if (srsInfo.isEmpty() || _srsOriginInfo.isEmpty())
	{
		QMessageBox::information(NULL, tr("Error"), tr("Invalid XML file!"));
		file.close();

		return;
	}

  // Get model origin
  auto  srs = osgEarth::SpatialReference::get(srsInfo.toStdString());

  osg::ref_ptr<osg::PositionAttitudeTransform>  model = new osg::PositionAttitudeTransform;
	model->setUserData(srs);

  QString  nodeName = XMLPath.split("/").back();
  model->setUserValue("filepath", pathXML.toLocal8Bit().toStdString());

  osg::Vec3d origin(_srsOriginInfo[0].toDouble(), _srsOriginInfo[1].toDouble(), _srsOriginInfo[2].toDouble() + zOffset);
  osgEarth::GeoPoint  geoOrigin = osgEarth::GeoPoint(srs, origin);
	geoOrigin = geoOrigin.transform(_globalSRS);

  auto anchorPoint = getNearestAnchorPoint(geoOrigin.vec3d());
  model->setPosition(geoOrigin.vec3d() - anchorPoint->getPosition());
  model->setUserValue("zOffset", zOffset);

  // Begin loading
  QFileInfoList  allFileList;
	getFolderFile(XMLPath, allFileList);

  emit        loadingProgress(10);
  LoadThread *loader = new LoadThread(_loadingLock, model, allFileList);
	connect(loader, &LoadThread::finished, loader, &QObject::deleteLater);
	connect(loader, &LoadThread::progress, this, &AddObliqueModel::loadingProgress);
  connect(loader, &LoadThread::done, [this, model, nodeName, geoOrigin]() {
    onLoadingDone(nodeName, model, geoOrigin);
  });

	loader->start();
}

void  AddObliqueModel::addObliqueModel()
{
  QStringList  XMLFileNames = QFileDialog::getOpenFileNames(nullptr, tr("Open File"), " ", tr("XML file(*.xml);;Allfile(*.*)"));

  for (auto path : XMLFileNames)
  {
    loadObliqueModel(path);
  }
}
