#ifndef ELEVATIONSLICESHOWDLG_H
#define ELEVATIONSLICESHOWDLG_H

#include "ui_ElevationSliceShowDlg.h"

#include <QDialog>
#include <QPoint>

#include <osg/Object>
#include <osg/Array>
#include <osg/Vec3>

#define  X_NOTE_COUNT 11
#define  Y_NOTE_COUNT 8

class ElevationSliceShowDlg : public QDialog
{
	Q_OBJECT

public:
	ElevationSliceShowDlg(QWidget *parent = 0);
	~ElevationSliceShowDlg();

signals:
	void cancelAnalysis();

public:
	void setShowPoint(osg::ref_ptr<osg::FloatArray> pointDisArr,osg::ref_ptr<osg::Vec3Array> pointsarr);

protected: 
	void closeEvent(QCloseEvent *event); 

private:
	void setDialogLayout();
	void  normalizeXValue(float* arr, float Xmin,float Xmax,float aimXmin,float aimXmax);
	void  normalizeZValue(float* arr, float Zmin,float Zmax,float aimZmin,float aimZmax);
protected:
	virtual void paintEvent( QPaintEvent * e);
	//void paint();
private:
	Ui::ElevationSliceShowDlg _ui;
	osg::ref_ptr<osg::Vec3Array> _pointsArr;
	QVector<QPoint> _linepoints;

	long _xvalues[X_NOTE_COUNT];//存储x轴的标注值
	float _zvalues[Y_NOTE_COUNT];//存储y轴的标注值

	float* _xCoorValues;//x坐标值
	float* _zCoorValues;//高度值

	int _xAxlStart;//x轴起点坐标（qt坐标系下）
	int _xAxlLength;//x轴长度（qt坐标系下）

	int _yAxlStart;//y轴起点坐标（qt坐标系下）
	int _yAxlLength;//y轴长度（qt坐标系下）

	int _xSpace;//x轴每个间隔单位长度
	int _ySpace;//y轴每个间隔单位长度

	float _xDistanceSum;
	osg::ref_ptr<osg::FloatArray> _pointDistanceArr;
};

#endif // ELEVATIONSLICESHOWDLG_H
