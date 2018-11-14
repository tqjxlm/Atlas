#ifndef MEASUREVOLUMEPLANESETDLG_H
#define MEASUREVOLUMEPLANESETDLG_H

#include <QDialog>
#include "ui_MeasureVolumePlaneSetDlg.h"

#include <osg/Array>

class MeasureVolumePlaneSetDlg : public QDialog
{
	Q_OBJECT

public:
	MeasureVolumePlaneSetDlg(QWidget *parent = 0);
	~MeasureVolumePlaneSetDlg();

	void setPlaneCoordinateInfo(osg::ref_ptr<osg::Vec3Array>  coorArr);
	osg::ref_ptr<osg::Vec3Array>  getPlanePoints();

private:
	Ui::MeasureVolumePlaneSetDlg _ui;
	osg::ref_ptr<osg::Vec3Array> _planeArry;

public slots:
void okBtClicked();
void cancelBtClicked();
};

#endif // MEASUREVOLUMEPLANESETDLG_H
