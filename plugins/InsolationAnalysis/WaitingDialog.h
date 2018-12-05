#ifndef WAITINGDIALOG_H
#define WAITINGDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QMovie;
class QLabel;
QT_END_NAMESPACE

class WaitingDialog :
	public QDialog
{
	 Q_OBJECT 

public:
	explicit WaitingDialog(const QString& str=tr("Loading..."),QWidget *parent = 0);
	~WaitingDialog(void);
	void startAnimation();

private: 
	QMovie* _movie; 
	QLabel* _labelMovie;
	QLabel* _labelTxt;

};

#endif
