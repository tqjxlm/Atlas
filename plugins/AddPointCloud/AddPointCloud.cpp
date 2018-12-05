#include "AddPointCloud.h"

#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QFileDialog>
#include <QTextStream>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QDockWidget>

#include <osg/BlendFunc>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/Texture2D>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osgDB/ReadFile>

AddPointCloud::AddPointCloud():
  _pointSize(3.0)
{
  _pluginCategory = "Data";
  _pluginName     = tr("Point Cloud Model");
}

AddPointCloud::~AddPointCloud()
{
}

void  AddPointCloud::setupUi(QToolBar *toolBar, QMenu *menu)
{
  QAction *addPCAction = new QAction(_mainWindow);

	addPCAction->setObjectName(QStringLiteral("addPCAction"));
  QIcon  icon5;
	icon5.addFile(QStringLiteral("resources/icons/point_cloud.png"), QSize(), QIcon::Normal, QIcon::Off);
	addPCAction->setIcon(icon5);
	addPCAction->setText(tr("Point Cloud"));
	addPCAction->setToolTip(tr("Load an Point Cloud model"));

	menu->addAction(addPCAction);
	toolBar->addAction(addPCAction);

	connect(addPCAction, SIGNAL(triggered()), this, SLOT(addPointCloud()));

	setupStyleTab();
}

void  AddPointCloud::setupStyleTab()
{
  auto  pcStyleTab = new QWidget();

	pcStyleTab->setObjectName(QStringLiteral("pcStyleTab"));
  auto  verticalLayout = new QVBoxLayout(pcStyleTab);
	pcStyleTab->setMaximumHeight(200);
	verticalLayout->setSpacing(6);
	verticalLayout->setContentsMargins(11, 11, 11, 11);
	verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
  auto  groupBox_5 = new QGroupBox(pcStyleTab);
	groupBox_5->setObjectName(QStringLiteral("groupBox_5"));
	groupBox_5->setTitle(tr("Point Style"));
  auto  gridLayout_8 = new QGridLayout(groupBox_5);
	gridLayout_8->setSpacing(6);
	gridLayout_8->setContentsMargins(11, 11, 11, 11);
	gridLayout_8->setObjectName(QStringLiteral("gridLayout_8"));
  auto  pcSizeSlider = new QSlider(groupBox_5);
	pcSizeSlider->setObjectName(QStringLiteral("pcSizeSlider"));
  QSizePolicy  sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Minimum);
	sizePolicy2.setHorizontalStretch(0);
	sizePolicy2.setVerticalStretch(0);
	sizePolicy2.setHeightForWidth(pcSizeSlider->sizePolicy().hasHeightForWidth());
	pcSizeSlider->setSizePolicy(sizePolicy2);
	pcSizeSlider->setMinimumSize(QSize(0, 0));
	pcSizeSlider->setMaximumSize(QSize(16777215, 25));
	pcSizeSlider->setMinimum(1);
	pcSizeSlider->setMaximum(15);
	pcSizeSlider->setSingleStep(1);
	pcSizeSlider->setPageStep(3);
	pcSizeSlider->setValue(3);
	pcSizeSlider->setOrientation(Qt::Horizontal);

	gridLayout_8->addWidget(pcSizeSlider, 0, 1, 1, 1);

  auto  pcSizeSpinBox = new QSpinBox(groupBox_5);
	pcSizeSpinBox->setObjectName(QStringLiteral("pcSizeSpinBox"));
	pcSizeSpinBox->setMinimumSize(QSize(42, 0));
	pcSizeSpinBox->setMaximumSize(QSize(16777215, 16777215));
	pcSizeSpinBox->setFrame(true);
	pcSizeSpinBox->setAlignment(Qt::AlignCenter);
	pcSizeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	pcSizeSpinBox->setMinimum(1);
	pcSizeSpinBox->setMaximum(15);
	pcSizeSpinBox->setValue(3);

	gridLayout_8->addWidget(pcSizeSpinBox, 0, 2, 1, 1);

  auto  label_10 = new QLabel(groupBox_5);
	label_10->setObjectName(QStringLiteral("label_10"));
  QSizePolicy  sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Minimum);
	sizePolicy1.setHorizontalStretch(0);
	sizePolicy1.setVerticalStretch(0);
	sizePolicy1.setHeightForWidth(label_10->sizePolicy().hasHeightForWidth());
	label_10->setSizePolicy(sizePolicy1);
	label_10->setMinimumSize(QSize(30, 0));
	label_10->setAlignment(Qt::AlignCenter);
	label_10->setText(tr("Size"));

	gridLayout_8->addWidget(label_10, 0, 0, 1, 1);

	verticalLayout->addWidget(groupBox_5);

  auto  verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

	verticalLayout->addItem(verticalSpacer_3);

  auto  controlPanel = _mainWindow->findChild<QDockWidget *>("controlPanel", Qt::FindDirectChildrenOnly);
  auto  tabWidget    = controlPanel->findChild<QTabWidget *>("tabWidget");

	if (controlPanel->maximumHeight() < 100)
  {
    controlPanel->setMaximumHeight(100);
  }

	connect(pcSizeSlider, SIGNAL(sliderMoved(int)), pcSizeSpinBox, SLOT(setValue(int)));
	connect(pcSizeSpinBox, SIGNAL(valueChanged(int)), pcSizeSlider, SLOT(setValue(int)));
	connect(pcSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setPointSize(int)));

	tabWidget->addTab(pcStyleTab, tr("Point Cloud"));
}

void  AddPointCloud::addPointCloud()
{
  QStringList  PCFileNames;

	PCFileNames = QFileDialog::getOpenFileNames(nullptr, tr("Open File"), " ", tr("LAS File(*.las);;Text file(*.txt);;Allfile(*.*)"));

	if (PCFileNames.isEmpty())
  {
    return;
  }

  for (auto fileName : PCFileNames)
  {
    loadPointCloudModel(fileName);
  }
}

void  AddPointCloud::setPointSize(int size)
{
	_pointSize = size;
  osg::ref_ptr<osg::Point>  point = new osg::Point();
	point->setSize(_pointSize);
	_pluginRoot->getStateSet()->setAttribute(point);
}

void  AddPointCloud::loadPointCloudModel(const QString& fileName)
{
	if (fileName.section(".", 1, 1) == "txt")
	{
    osg::ref_ptr<osg::PositionAttitudeTransform>  pcModel = new osg::PositionAttitudeTransform;
		_currentAnchor->addChild(pcModel);

    osg::Geode     *pcGeode  = new osg::Geode();
    osg::Geometry  *pcGeom   = new osg::Geometry();
		osg::Vec3Array *pcCoords = new osg::Vec3Array();
		osg::Vec4Array *pcColors = new osg::Vec4Array();

		/** Formula for the two spirals */

    unsigned int  i       = 0;
    float         pbValue = 0;
    emit          loadingProgress(0);
    unsigned int  ptCount = 0;
    QString       ptLine;
    QFile         file(fileName);

		if (file.open(QFile::ReadOnly | QIODevice::Text))
		{
      QTextStream  data(&file);

			while (!data.atEnd())
			{
				ptLine = data.readLine();

				if (ptLine.isEmpty())
        {
          break;
        }

        if (!ptLine.contains(","))
				{
					ptLine.remove('\n');
					ptCount = ptLine.toUInt();
					continue;
				}
				else
				{
					if (ptLine.section(",", 0, 0) == "X")
          {
            continue;
          }
          else
					{
            float  pt_x = ptLine.section(",", 0, 0).toFloat();
            float  pt_y = ptLine.section(",", 1, 1).toFloat();
            float  pt_z = ptLine.section(",", 2, 2).toFloat();
            float  pt_r = ptLine.section(",", 7, 7).toFloat();
            float  pt_g = ptLine.section(",", 8, 8).toFloat();
            float  pt_b = ptLine.section(",", 9, 9).toFloat();

						pcCoords->push_back(osg::Vec3(pt_x, pt_y, pt_z));
						pcColors->push_back(osg::Vec4(pt_r / 255, pt_g / 255, pt_b / 255, 1.0f));

						i++;
						pbValue = 100 * i / ptCount;
            emit  loadingProgress(pbValue);
					}
				}
			}
		}

		pcGeom->setVertexArray(pcCoords);
		pcGeom->setColorArray(pcColors, osg::Array::BIND_PER_VERTEX);
		pcGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, pcCoords->size()));

		osg::Vec3Array *normals = new osg::Vec3Array;
		normals->push_back(osg::Vec3(0.0f, 1.0f, 0.0f));
		pcGeom->setNormalArray(normals);
		pcGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);

		pcGeode->addDrawable(pcGeom);

		osg::StateSet *set = new osg::StateSet();

		set->setMode(GL_BLEND, osg::StateAttribute::ON);
		osg::BlendFunc *fn = new osg::BlendFunc();
		fn->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::DST_ALPHA);
		set->setAttributeAndModes(fn, osg::StateAttribute::ON);

		osg::Point *point = new osg::Point();
		point->setSize(_pointSize);
		set->setAttribute(point);
		set->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);

		set->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
		set->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

		pcModel->addChild(pcGeode);
		pcModel->setStateSet(set);

    emit  loadingDone();

		recordNode(pcModel, fileName.split("/").back().section(".", 0, 0));
		pcModel->setUserValue("filepath", fileName.toLocal8Bit().toStdString());
	}
  else if ((fileName.section(".", 1, 1) == "las") || (fileName.section(".", 1, 1) == "laz"))
	{
    osg::ref_ptr<osg::PositionAttitudeTransform>  pcModel = new osg::PositionAttitudeTransform;
		_currentAnchor->addChild(pcModel);

    osg::ref_ptr<osg::Node>  pointCloud = osgDB::readNodeFile(fileName.toLocal8Bit().toStdString());

		if (pointCloud.valid())
		{
			pcModel->addChild(pointCloud);

			osg::StateSet *set = new osg::StateSet();

			set->setMode(GL_BLEND, osg::StateAttribute::ON);
			osg::BlendFunc *fn = new osg::BlendFunc();
			fn->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::DST_ALPHA);
			set->setAttributeAndModes(fn, osg::StateAttribute::ON);

			osg::Point *point = new osg::Point();
			point->setSize(_pointSize);
			set->setAttribute(point);
			set->setMode(GL_POINT_SMOOTH, osg::StateAttribute::ON);

			set->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
			set->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

			pcModel->setStateSet(set);

			recordNode(pcModel, fileName.split("/").back().section(".", 0, 0));

			pcModel->setUserValue("filepath", fileName.toLocal8Bit().toStdString());

			pcModel->setUserValue("isPoint", 1);
		}
	}
}
