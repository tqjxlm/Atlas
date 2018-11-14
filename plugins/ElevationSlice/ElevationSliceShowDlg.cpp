#include "ElevationSliceShowDlg.h"

#include <QDesktopWidget>
#include <QPainter>
#include <QPen>


ElevationSliceShowDlg::ElevationSliceShowDlg(QWidget *parent)
	: QDialog(parent)
{
	_ui.setupUi(this);

	this->setWindowTitle(tr("Elevation slice results"));
	this->setFixedSize(500,370);

	setWindowFlags (Qt::WindowStaysOnTopHint);
	setWindowModality(Qt::WindowModal);

	QRect deskRect = QApplication::desktop()->availableGeometry();
	this->move(deskRect.left()+20, deskRect.bottom()-450);

	_xAxlStart=35;
	_xAxlLength=450;

	 _yAxlStart=35;
	 _yAxlLength=290;

	 _xSpace=40;
	 _ySpace=35;

	 _xDistanceSum = 0;
	 _pointDistanceArr = NULL;
	 _pointsArr = NULL;
	 for (int i = 0; i < X_NOTE_COUNT; i++)
		 _xvalues[i] = 0;
	 for (int i = 0; i < Y_NOTE_COUNT; i++)
		 _zvalues[i] = 0;
}

ElevationSliceShowDlg::~ElevationSliceShowDlg()
{
	
}
void ElevationSliceShowDlg::closeEvent(QCloseEvent *event)
{
	emit cancelAnalysis();
}
void ElevationSliceShowDlg::paintEvent( QPaintEvent * e)
{
	QPainter painter(this);
	QPen pen; 
	pen.setWidth(2); 
	pen.setBrush(Qt::blue); 
	painter.setRenderHint(QPainter::Antialiasing);

	painter.setPen(pen);

	painter.drawLine(_xAxlStart,_yAxlStart,_xAxlStart,_yAxlStart+_yAxlLength);  //绘制y轴
	painter.drawLine(_xAxlStart,_yAxlStart+_yAxlLength,_xAxlStart+_xAxlLength,_yAxlStart+_yAxlLength); //绘制x轴

	int i;
	int index=0;
	for(i = _xAxlStart;i<_xAxlStart+_xAxlLength;i+=_xSpace)//x zhou
	{
		painter.drawLine(i+_xSpace,_yAxlStart+_yAxlLength,i+_xSpace,_yAxlStart+_yAxlLength-10); //绘制x轴上的点
		painter.drawText(i+_xSpace-5,_yAxlStart+_yAxlLength+15,QString::number(_xvalues[index++])); //绘制文本

	}
	index=0;
	for (i=_yAxlStart+_yAxlLength-_ySpace;i>=_yAxlStart;i-=_ySpace)
	{
		painter.drawLine(_xAxlStart,i,_xAxlStart+10,i); //绘制y轴上的点
		painter.drawText(_xAxlStart-30,i+5,QString::number(_zvalues[index++],'f', 1)); //绘制文本
	}

	painter.drawText(5,20,"高度值(m)");
	painter.drawText(_xAxlStart+_xAxlLength-30,_yAxlStart+_yAxlLength+30,"间距(m)");

	QPainter painterline(this);
	QPen penline; 
	penline.setWidth(2); 
	penline.setBrush(Qt::red); 
	painter.setRenderHint(QPainter::Antialiasing);

	painterline.setPen(penline);
    painterline.drawPolyline(_linepoints);

	ElevationSliceShowDlg::update();
}
void ElevationSliceShowDlg::setDialogLayout()
{

}
void ElevationSliceShowDlg::setShowPoint(osg::ref_ptr<osg::FloatArray> pointDisArr,osg::ref_ptr<osg::Vec3Array> pointsarr)
{
	_pointsArr = pointsarr;
	_pointDistanceArr = pointDisArr;
	_linepoints.clear();

	//for (int i=0;i<pointsarr->size();i++)
	//{
	//	qDebug()<<pointsarr->at(i).z();
	//}

	//获得采样点x坐标、z高度值的最大值最小值
	float xmin=10000000;float xmax=-10000000;
	float zmin=10000;float zmax=-10000;
	osg::Vec3 posmin;osg::Vec3 posMax;

	for (int i=0;i<_pointsArr->size();i++)
	{
		if (_pointsArr->at(i).x()<xmin)
		{
			xmin = _pointsArr->at(i).x();
			posmin = _pointsArr->at(i);
		}
		if (_pointsArr->at(i).z()<zmin)
		{
			zmin = _pointsArr->at(i).z();
		}
		if (_pointsArr->at(i).x()>xmax)
		{
			xmax = _pointsArr->at(i).x();
			posMax = _pointsArr->at(i);
		}
		if (_pointsArr->at(i).z()>zmax)
		{
			zmax = _pointsArr->at(i).z();
		}
	}
	
	_xDistanceSum = _pointDistanceArr->at(_pointDistanceArr->size() - 1);
	//for (int i = 0; i < _pointsArr->size() - 1; i++)
	//{
	//	_xDistanceSum += (_pointsArr->at(i) - _pointsArr->at(i + 1)).length();
	//}
	//设置x轴和y轴的标注值
	float xinterval = (float)_xDistanceSum /(X_NOTE_COUNT-1);
	for (int j=0;j<X_NOTE_COUNT;j++)
	{
		_xvalues[j]=j*xinterval;//x轴算起点与终点的距离
	}

	float zinterval = (zmax-zmin)/(Y_NOTE_COUNT-1);
	for (int j=0;j<Y_NOTE_COUNT;j++)
	{
		_zvalues[j]=zmin+j*zinterval;//y轴算从小到大的高度值
	}

	//归一化采样点x地理坐标到qt坐标
	 _xCoorValues= new float[_pointsArr->size()];
	 normalizeXValue(_xCoorValues,xmin,xmax,_xAxlStart+_xSpace,_xAxlStart+_xAxlLength);

	//归一化采样点z高度值到qt坐标
	_zCoorValues = new float[_pointsArr->size()];
	normalizeZValue(_zCoorValues,zmin,zmax,_yAxlStart,_yAxlStart+_yAxlLength-_ySpace);//zAimmax为

	//加入画点中
	for (int i=0;i<_pointsArr->size();i++)
	{
		QPoint po(_xCoorValues[i],_zCoorValues[i]);
		_linepoints.push_back( po);
	}

}

//归一化x坐标值到aimXmin——aimXmax
void  ElevationSliceShowDlg::normalizeXValue(float* arr, float Xmin,float Xmax,float aimXmin,float aimXmax)
{
	int length = _pointsArr->size();
	float k = (aimXmax - aimXmin) / (_xDistanceSum-0);
	for (int i = 0; i < length; i++)
	{
		arr[i] = aimXmin + k*(_pointDistanceArr->at(i)-0);

	}
	//int length = _pointsArr->size();
	//float k=(aimXmax-aimXmin)/(Xmax-Xmin);
	//for (int i=0;i<length;i++)
	//{
	//	arr[i]=aimXmin+k*(_pointsArr->at(i).x()-Xmin);

		//if (_pointsArr->at(length - 1).x()<_pointsArr->at(0).x())//如果起点x坐标大于终点x坐标，则反转数组
		//{
		//	arr[i] = aimXmin + aimXmax - arr[i];
		//}

	//}
}

void  ElevationSliceShowDlg::normalizeZValue(float* arr, float Zmin,float Zmax,float aimZmin,float aimZmax)
{

	int length = _pointsArr->size();
	float k=(aimZmax-aimZmin)/(Zmax-Zmin);//归一化系数
	//qDebug()<<"max z:"<<Zmax<<endl;
	for (int i=0;i<length;i++)
	{
		float tmp=aimZmin+k*(_pointsArr->at(i).z()-Zmin);//归一化后的数值
		arr[i]=aimZmin+aimZmax-tmp;//计算在qt坐标系中的坐标
		//if (arr[i]<aimZmin)//如果低于最小坐标值
		//{
		//	arr[i]=aimZmin;
		//}
	}
}