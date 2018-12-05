#include "TileSelectDialog.h"

#include "ui_TileSelectDialog.h"

#include <QDesktopWidget>

TileSelectDialog::TileSelectDialog(int itemTotalCount,QWidget *parent)
	: QWidget(parent)
{
    _ui = new Ui::TileSelectDialog;

	_ui->setupUi(this);
	setWindowFlags(Qt::WindowStaysOnTopHint);
	setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose);

	QRect deskRect = QApplication::desktop()->availableGeometry();
	this->move(deskRect.left() + 200, deskRect.top() + 200);

	connect(_ui->selectAllBt, SIGNAL(clicked()), this, SLOT(okAllBtClicked()));
	connect(_ui->unselectAllBt, SIGNAL(clicked()), this, SLOT(cancleAllBtClicked()));

	_itemTotalCount = itemTotalCount;
}

TileSelectDialog::~TileSelectDialog()
{

}

void TileSelectDialog::selectTileSlot(const QString& tileName)
{
	QListWidgetItem *item = new QListWidgetItem(tileName);
	QList<QListWidgetItem*> list = _ui->listWidget->findItems(tileName, Qt::MatchFixedString);
	if (list.count() > 0)
		return;
	_ui->listWidget->addItem(item);
	updateLabelCount();
}

void TileSelectDialog::unselectTileSlot(const QString& tileName)
{
	QList<QListWidgetItem*> list=_ui->listWidget->findItems(tileName, Qt::MatchFixedString);
	QListWidgetItem* item = list.at(0);
	int rownum = _ui->listWidget->row(item);
	_ui->listWidget->takeItem(rownum);
	updateLabelCount();
}

void TileSelectDialog::okAllBtClicked()
{
	emit selectAllTile();
	updateLabelCount();
}

void TileSelectDialog::cancleAllBtClicked()
{
	_ui->listWidget->clear();
	emit unselectAllTile();
	updateLabelCount();
}

void TileSelectDialog::updateLabelCount()
{
	QString labelstring = QString("%1/%2").arg(QString::number(_ui->listWidget->count())).arg(QString::number(_itemTotalCount));
	_ui->label_count->setText(labelstring);
}

void  TileSelectDialog::closeEvent(QCloseEvent *e)
{
	cancleAllBtClicked();
    emit closed();
}
