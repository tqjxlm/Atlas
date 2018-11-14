#include "WaitingDialog.h"

#include <QBitmap>
#include <QMovie>
#include <QLabel>
#include <QVBoxLayout>

WaitingDialog::WaitingDialog(QString str, QWidget*parent) :
	QDialog(parent)
{
	QPixmap pixmap("resources/icons/bgWaiting.png");
	setMask(pixmap.mask());
	this->setAutoFillBackground(true);
	resize(150, 150);

	_labelMovie = new QLabel(this);
	_labelMovie->setAlignment(Qt::AlignCenter);
	_movie = new QMovie("resources/icons/loading.gif");
	_labelMovie->setMovie(_movie);
	_labelMovie->setAlignment(Qt::AlignHCenter);

	_labelTxt = new QLabel(str);
	_labelTxt->setAlignment(Qt::AlignHCenter);
	QFont ft;
	ft.setPointSize(10);
	_labelTxt->setFont(ft);

	QVBoxLayout *dlgLayout = new QVBoxLayout;
	dlgLayout->setMargin(20);
	dlgLayout->setSpacing(5);
	dlgLayout->addWidget(_labelMovie);
	dlgLayout->addWidget(_labelTxt);

	setLayout(dlgLayout);
}

WaitingDialog::~WaitingDialog(void)
{
	_movie->stop();
	delete _labelMovie;
	delete _movie;
}

void WaitingDialog::startAnimation()
{
	_movie->start();
}