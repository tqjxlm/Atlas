#ifndef FEATURESTYLESETTINGDLG_H
#define FEATURESTYLESETTINGDLG_H

#include <QDialog>
#include "ui_FeatureStyleSettingDlg.h"
#include<qmap.h>

class FeatureStyleSettingDlg : public QDialog
{
	Q_OBJECT

public:
	FeatureStyleSettingDlg(QWidget *parent = 0);
	~FeatureStyleSettingDlg();

	void setLayerStyle(std::string gemtype,float layerHeight,const QString& layerName, float layerHeightPre);

	QString getIconPath() { return _iconPath; }
	float getLayerHeight() { return _layerHeight; }
public slots:
	void OkButtonClicked();
	void CancelBtClicked();

private:
	Ui::FeatureStyleSettingDlg _ui;

	QString _iconPath;
	float _layerHeight;
};

#endif // FEATURESTYLESETTINGDLG_H
