#pragma once
#ifndef URLDIALOG_HPP
#define URLDIALOG_HPP
#include <QDialog>

#include "ui_urlDialog.h"

class urlDialog : public QDialog {
	Q_OBJECT

public:
	urlDialog(QMap<QString, QString> examples, QWidget * parent = nullptr);
	~urlDialog();

	QString getUrl();

private slots:
	void pickExample(QString name);
	void resetComboBox(const QString &);

private:
	Ui_urlDialog _ui;
	QMap<QString, QString> _examples;
};

#endif // URLDIALOG_HPP