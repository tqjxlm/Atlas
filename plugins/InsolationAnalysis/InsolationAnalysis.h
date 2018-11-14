#pragma once
#include <QtPlugin>
#include <TileSelect/TileSelect.h>

QT_BEGIN_NAMESPACE
class QToolBar;
class QAction;
class QMenu;
QT_END_NAMESPACE

class TimeIntervalDlg;

class InsolationAnalysis : public TileSelect
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.tqjxlm.Atlas.PluginInterface" FILE "InsolationAnalysis.json")
	Q_INTERFACES(PluginInterface)

public:
	InsolationAnalysis();
	~InsolationAnalysis();
	virtual void setupUi(QToolBar *toolBar, QMenu *menu) override;
	virtual void onLeftButton() override;
	virtual void onMouseMove() override;
	virtual void onRightButton() override;
    virtual void loadContextMenu(QMenu* contextMenu, QTreeWidgetItem* selectedItem) override;

protected:
	virtual void toggle(bool checked) override;

	time_t convert_str_to_tm(const char * str_time);
	osg::Vec2 getSolarAngle(const char* str_time1, int N, double lon, double lat);
	osg::Vec3 rayEquation(osg::Vec3 p, double azimuthAngle, double heightAngle);
	float getIllumination(QString start_time, QString end_time, int step, osg::Vec3 pointA, double lon, double lat);
	float getTotalMin();
    void initLineGeode();
    bool makeSAArray(osg::Vec3 a, osg::Vec3 b);
    bool makeShpFile();
    osg::Vec3 worldPos2GeoPos(osg::Vec3 worldPos);

protected slots:
	void finished();
    void okButton4SASlot();
    void cancelButton4SASlot();

protected:
	TimeIntervalDlg *_timeIntervalDlg;

	QAction* _action;

	osg::Vec3 _startVertex, _endVertex;//offsetWorldPos
	osg::Vec3 _startVertexCWP, _endVertexCWP;//currentWorldPos
	osg::ref_ptr<osg::Geode> _diagonalLineGeode;
	osg::ref_ptr<osg::Geometry> _diagonalLineGeometry;
	osg::ref_ptr<osg::Vec3Array> _diagonalLineVertex;

	osg::Vec3 _minVertex;
	osg::Vec3 _maxVertex;
	int _nArrayWidth;
	int _nArrayHeight;

	float* _SAArray;
	int _nIdx;
	osg::Vec3 _first_intersection;

	QString _shpPath;
	QString _SAPath;
	double _SAInterval;

	bool _isStopped;

	osg::Vec3 _pointA;
	osg::Vec3 _geo_pointA;
	osg::Vec3 _pointB;

	osg::Vec2 _twoAngles;

	QString _startTime;
	QString _endTime;
	float _totalMin;

	int _N;
};
