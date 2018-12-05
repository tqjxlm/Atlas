#include "SlopAnalysis.h"

#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QToolBar>

#include <osg/PositionAttitudeTransform>
#include <osgSim/OverlayNode>
#include <osgUtil/SmoothingVisitor>
#include <osgQt/GraphicsWindowQt>
#include <osgDB/Registry>

#include <DataManager/FindNode.hpp>
#include <ViewerWidget/ViewerWidget.h>
#include <DataManager/DataManager.h>
#include <DataManager/DataRecord.h>

static osg::Vec4 colorToVec(const QColor &color)
{
  return osg::Vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

static QColor vecToColor(const osg::Vec4 &vec)
{
  return QColor(vec[0] * 255, vec[1] * 255, vec[2] * 255, vec[3] * 255);
}

class SmoothingCallback : public osgDB::Registry::ReadFileCallback
{
public:
  virtual osgDB::ReaderWriter::ReadResult readNode(const std::string& fileName, const osgDB::ReaderWriter::Options* options) override
  {
    osgDB::ReaderWriter::ReadResult result = osgDB::Registry::instance()->readNodeImplementation(fileName, options);

    osg::Node* loadedNode = result.getNode();
    if (loadedNode)
    {
      osgUtil::SmoothingVisitor smoothing;
      loadedNode->accept(smoothing);
    }
    return result;
  }
};

SlopAnalysis::SlopAnalysis()
{
  _pluginName = tr("Slop Analysis");
  _pluginCategory = "Analysis";
}

SlopAnalysis::~SlopAnalysis()
{
}

void SlopAnalysis::setupUi(QToolBar * toolBar, QMenu * menu)
{
  _action = new QAction(_mainWindow);
  _action->setObjectName(QStringLiteral("slopAnalysisAction"));
  _action->setCheckable(true);
  QIcon icon19;
  icon19.addFile(QStringLiteral("resources/icons/angle.png"), QSize(), QIcon::Normal, QIcon::Off);
  _action->setIcon(icon19);
  _action->setText(tr("Slop"));
  _action->setToolTip(tr("Slop Analysis"));

  connect(_action, SIGNAL(toggled(bool)), this, SLOT(toggle(bool)));

  toolBar->addAction(_action);
  menu->addAction(_action);
}

void SlopAnalysis::toggle(bool checked)
{
  osg::ref_ptr<osg::Node> analyzedNode = _dataRoot;
  auto state = analyzedNode->getOrCreateStateSet();

  if (checked)
  {
    // Get color settings
    auto colors = getOrAddPluginSettings("Slop colors", QList<QVariant>{
      QColor{ 38, 167, 0 },
        QColor{ 142, 207, 0 },
        QColor{ 255, 255, 2 },
        QColor{ 255, 160, 0 },
        QColor{ 255, 70, 0 },
    }).toList();

      // Set OpenGL context
      _mainViewer->getMainContext()->getState()->setUseModelViewAndProjectionUniforms(true);
      osg::Uniform *colorArray = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "slopeColors", 5);
      for (unsigned i = 0; i < 5; i++)
        colorArray->setElement(i, colorToVec(colors[i].value<QColor>()));
      state->addUniform(colorArray);

      // Keep normal vector updated
      osgUtil::SmoothingVisitor smoothing;
      analyzedNode->accept(smoothing);
      osgDB::Registry::instance()->setReadFileCallback(new SmoothingCallback);

      // Load shaders
      osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX);
      osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT);

      _slopeProgram = new osg::Program;
      _slopeProgram->addShader(vertShader);
      _slopeProgram->addShader(fragShader);

      if (!vertShader->loadShaderSourceFromFile("Resources/shaders/slope.vert"))
      {
        osg::notify(osg::WARN) << "vertex shader load failed" << std::endl;
        return;
      }

      if (!fragShader->loadShaderSourceFromFile("Resources/shaders/slope.frag"))
      {
        osg::notify(osg::WARN) << "fragment shader load failed" << std::endl;
        return;
      }

      state->setAttribute(_slopeProgram, osg::StateAttribute::ON);

      // Draw a legend
      QVector<osg::Vec4> colorVec;
      for (unsigned i = 0; i < 5; i++)
        colorVec.append(colorToVec(colors[i].value<QColor>()));

      QVector<QString> txtVec;
      txtVec.append("<=2");
      txtVec.append("2~6");
      txtVec.append("6~15");
      txtVec.append("15~30");
      txtVec.append("30~90");

      _root->addChild(ViewerWidget::createLegendHud(tr("Height Legend"), colorVec, txtVec));
  }
  else
  {
    // Recover states
    state->setAttribute(_slopeProgram, osg::StateAttribute::OFF);
    state->removeAttribute(osg::StateAttribute::PROGRAM);

    osgDB::Registry::instance()->setReadFileCallback(NULL);

    _mainViewer->getMainContext()->getState()->setUseModelViewAndProjectionUniforms(false);
    _root->removeChild(findNodeInNode("hudCamera", _root));
  }
}
