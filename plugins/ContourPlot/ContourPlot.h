#pragma once
#include <QtPlugin>
#include <PluginInterface/PluginInterface.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class ContourPlot : public PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "ContourPlot.json")
	Q_INTERFACES(PluginInterface)

public:
	ContourPlot();
	~ContourPlot();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;

	virtual void onLeftButton();
	virtual void onMouseMove();
	virtual void onRightButton();
	virtual void onReleaseButton();

	void initLineGeode();
	bool makeDEMArray(osg::Vec3 a, osg::Vec3 b);
	bool makeShpFile();

public slots:
	void contourPlotFunc();

protected:
	QAction* _action;

	bool _isAnalysis;

	osg::Vec3 _startVertex, _endVertex;
	osg::Vec3 _startVertexCWP, _endVertexCWP;
	osg::ref_ptr<osg::Geode> _diagonalLineGeode;
	osg::ref_ptr<osg::Geometry> _diagonalLineGeometry;
	osg::ref_ptr<osg::Vec3Array> _diagonalLineVertex;

	osg::Vec3 _minVertex;
	osg::Vec3 _maxVertex;
	int _nDemWidth;
	int _nDemHeight;
	const char* _wkt;

	float* _demArray;
	int _nIdx;
	osg::Vec3 _first_intersection;

	QString _shpPath;
	QString _demPath;
	double _dfContourInterval;

	bool _isStopped;
};
