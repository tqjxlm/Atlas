#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class OGRPolygon;
class OGRLineString;

namespace osgText
{
class Text;
}

namespace osg
{
class Geode;
class Geometry;
}

class DrawVector: public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "DrawVector.json")
	Q_INTERFACES(PluginInterface)

public:
	DrawVector();

	~DrawVector();

  virtual void                 setupUi(QToolBar *toolBar, QMenu *menu) override;

  void                         OpenVectorFile(const QString &path);

public slots:
  void                         DrawVectorFromFile(QString filePath, QString nodeName = "ffg");

protected:
  osg::Vec3                    MiddlePointOfPolygon(osg::ref_ptr<osg::Vec3Array> pointsArr, osg::Vec3 centerpoint);

  osg::ref_ptr<osg::Geometry>  createPolyGem(osg::ref_ptr<osg::Vec3Array> polygonArr);

  osg::ref_ptr<osg::Geometry>  createLineGem(osg::ref_ptr<osg::Vec3Array> polygonArr);

  osg::ref_ptr<osgText::Text>  createTextGeode(std::string &txt, osg::Vec3 position);

  void                         createFeatureNoteText(QMap<int, QString> featurefieldmap, QMap<int, osg::Vec3> featuregeomMap,
                                                     osg::ref_ptr<osg::Group> labelGroup);

  osg::ref_ptr<osg::Geode>     drawPolygon(OGRPolygon *poly, osg::Vec3 &centerpoint);

  osg::ref_ptr<osg::Geometry>  drawLineString(OGRLineString *linestring);

  void                         convtGeoCoorToProCoor(osg::Vec3 &proPos, osg::Vec3 geoPos);

  void                         styleSetting(const QString &name);

  void                         lodSetting(double dis, osg::Group *vectorGroup, const QString &name);

protected:
  QAction                  *_action;
  osg::ref_ptr<osg::LOD>    _vectorLod;
  osg::ref_ptr<osg::Group>  _labelGroup;
  float                     _vecFontSize;
  int                       _highestVisibleHeight;
  unsigned int              _textCharactorMode;
  char                     *_origlSRS;
  osg::Group               *_drawVectorRoot;
  osg::Vec4                 lineColor;
  osg::Vec4                 _textColor;
  osg::Vec4                 _pointColor;
};
