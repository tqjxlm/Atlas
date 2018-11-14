#include "MeasureVolumePlaneSetDlg.h"

MeasureVolumePlaneSetDlg::MeasureVolumePlaneSetDlg(QWidget *parent)
	: QDialog(parent)
{
	_ui.setupUi(this);

	setWindowModality(Qt::WindowModal);

	connect(_ui.okBt, SIGNAL(clicked()), SLOT(okBtClicked()));
	connect(_ui.cancelBt, SIGNAL(clicked()), SLOT(cancelBtClicked()));
	_planeArry = new osg::Vec3Array;
}

MeasureVolumePlaneSetDlg::~MeasureVolumePlaneSetDlg()
{

}

void MeasureVolumePlaneSetDlg::setPlaneCoordinateInfo(osg::ref_ptr<osg::Vec3Array> coorArr)
{
	osg::Vec3 coor1 = coorArr->at(0);
	osg::Vec3 coor2 = coorArr->at(1);
	osg::Vec3 coor3 = coorArr->at(2);

	_ui.coord1_X->setText(QString::number(coor1.x(), 'f', 2));
	_ui.coord1_Y->setText(QString::number(coor1.y(), 'f', 2));
	_ui.coord1_Z->setText(QString::number(coor1.z(), 'f', 2));

	_ui.coord2_X->setText(QString::number(coor2.x(), 'f', 2));
	_ui.coord2_Y->setText(QString::number(coor2.y(), 'f', 2));
	_ui.coord2_Z->setText(QString::number(coor2.z(), 'f', 2));

	_ui.coord3_X->setText(QString::number(coor3.x(), 'f', 2));
	_ui.coord3_Y->setText(QString::number(coor3.y(), 'f', 2));
	_ui.coord3_Z->setText(QString::number(coor3.z(), 'f', 2));
}

osg::ref_ptr<osg::Vec3Array> MeasureVolumePlaneSetDlg::getPlanePoints()
{
	return _planeArry;
}

void MeasureVolumePlaneSetDlg::okBtClicked()
{
	float x1 = _ui.coord1_X->text().toFloat();
	float y1 = _ui.coord1_Y->text().toFloat();
	float z1 = _ui.coord1_Z->text().toFloat();

	float x2 = _ui.coord2_X->text().toFloat();
	float y2 = _ui.coord2_Y->text().toFloat();
	float z2 = _ui.coord2_Z->text().toFloat();

	float x3 = _ui.coord3_X->text().toFloat();
	float y3 = _ui.coord3_Y->text().toFloat();
	float z3 = _ui.coord3_Z->text().toFloat();

	osg::Vec3 pos1(x1, y1, z1);
	osg::Vec3 pos2(x2, y2, z2);
	osg::Vec3 pos3(x3, y3, z3);

	_planeArry->clear();
	_planeArry->push_back(pos1);
	_planeArry->push_back(pos2);
	_planeArry->push_back(pos3);
	

	this->accept();
}

void MeasureVolumePlaneSetDlg::cancelBtClicked()
{
	this->close();
}