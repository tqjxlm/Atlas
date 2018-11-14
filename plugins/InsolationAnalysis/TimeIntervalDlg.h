#ifndef TIMEINTERVALDLG_H
#define TIMEINTERVALDLG_H

#include <QDialog>
#include <QString>

#include "ui_TimeIntervalDlg.h"

class TimeIntervalDlg : public QDialog
{
	Q_OBJECT

public:
	TimeIntervalDlg();
	~TimeIntervalDlg();
	QString getStartTimeString() { return _start_time; }
	QString getEndTimeString() { return _end_time; }

public slots:
	void okButtonClicked();
	void cancelButtonClicked();

private:
	Ui::TimeIntervalDlg _ui;
	QString _start_time, _end_time;//日照的起始时间&终止时间
};

#endif // TIMEINTERVALDLG_H
