#include "TimeIntervalDlg.h"

#include <QDateTime>

TimeIntervalDlg::TimeIntervalDlg()
{
	_ui.setupUi(this);

	_ui.startDateTimeEdit->setDateTime(QDateTime(QDate::currentDate().addDays(-1), QTime::currentTime()));
	_ui.endDateTimeEdit->setDateTime(QDateTime(QDate::currentDate(), QTime::currentTime()));

	connect(_ui.okPushButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(_ui.cancelPushButton, SIGNAL(clicked()), this, SLOT(reject()));

	connect(_ui.okPushButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
	connect(_ui.cancelPushButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));
}

TimeIntervalDlg::~TimeIntervalDlg()
{}

void TimeIntervalDlg::okButtonClicked()
{
	//从LineEdit获取起始时间
	_start_time = _ui.startDateTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm");
	//从LineEdit获取终止时间
	_end_time = _ui.endDateTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm");
	
	//添加秒数
	_start_time = _start_time + ":00";
	_end_time = _end_time + ":00";
}
void TimeIntervalDlg::cancelButtonClicked()
{
	_start_time = "";
	_end_time = "";
}