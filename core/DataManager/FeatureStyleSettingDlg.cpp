#include "FeatureStyleSettingDlg.h"

#include <QIcon>

FeatureStyleSettingDlg::FeatureStyleSettingDlg(QWidget *parent)
	: QDialog(parent)
{
	_ui.setupUi(this);

	_ui.iconComboBox->insertItem(0, QIcon("Resources/circle1.png"), "", "geometry");
	_ui.iconComboBox->insertItem(1, QIcon("Resources/rec1.png"), "","Resources/rec.png");
	_ui.iconComboBox->insertItem(2, QIcon("Resources/star1.png"), "", "Resources/star.png");
	

	connect(_ui.okButton, SIGNAL(clicked()), SLOT(OkButtonClicked()));
	connect(_ui.cancelButton, SIGNAL(clicked()), SLOT(CancelBtClicked()));

	_ui.lyHeightTextEdit->setPlaceholderText("0");
}

FeatureStyleSettingDlg::~FeatureStyleSettingDlg()
{

}

void FeatureStyleSettingDlg::setLayerStyle(std::string gemtype, float layerHeight, const QString& layerName,float layerHeightPre)
{
	if (QString::fromStdString(gemtype).contains("Point")|| QString::fromStdString(gemtype).contains("Icon"))
		_ui.iconComboBox->setEnabled(true);
	else
		_ui.iconComboBox->setEnabled(false);

	_ui.lyHeightTextEdit->setPlaceholderText(QString::number(layerHeightPre, 'f', 1));

	_ui.lyHeightTextEdit->setText("");
}

void FeatureStyleSettingDlg::OkButtonClicked()
{
	int  index = _ui.iconComboBox->currentIndex();
	QVariant va=_ui.iconComboBox->itemData(index);
	_iconPath = va.value<QString>();

	_layerHeight = _ui.lyHeightTextEdit->toPlainText().toFloat();

	this->accept();
}
void FeatureStyleSettingDlg::CancelBtClicked()
{
	this->hide();
}