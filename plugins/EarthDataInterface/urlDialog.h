#pragma once
#ifndef URLDIALOG_HPP
#define URLDIALOG_HPP

#include <QDialog>
#include <QMap>

#include "EarthDataInterface_global.h"

class Ui_urlDialog;

class EARTHDATAINTERFACE_EXPORT urlDialog : public QDialog {
  Q_OBJECT

public:
	urlDialog(QMap<QString, QString> examples, QWidget * parent = nullptr);
	~urlDialog();

	QString getUrl();

private slots:
	void pickExample(const QString& name);
	void resetComboBox(const QString&);

private:
	Ui_urlDialog* _ui;
	QMap<QString, QString> _examples;
};

#endif // URLDIALOG_HPP