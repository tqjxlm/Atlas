#include "urlDialog.h"

#include "ui_urlDialog.h"

#include <QComboBox>

urlDialog::urlDialog(QMap<QString, QString> examples, QWidget * parent)
	: QDialog(parent)
	, _examples(examples)
{
  _ui = new Ui_urlDialog();
	_ui->setupUi(this);

	_ui->comboBox->addItem("");
	_ui->comboBox->addItems(examples.keys());

	connect(_ui->comboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(pickExample(const QString&)));
	connect(_ui->urlEdit, SIGNAL(textEdited(const QString&)), this, SLOT(resetComboBox(const QString&)));
}

urlDialog::~urlDialog() 
{
	
}

QString urlDialog::getUrl()
{
	return _ui->urlEdit->text();
}

void urlDialog::resetComboBox(const QString&)
{
	_ui->comboBox->setCurrentIndex(0);
}

void urlDialog::pickExample(const QString& name)
{
	if (!name.isEmpty())
		_ui->urlEdit->setText(_examples[name]);
}
