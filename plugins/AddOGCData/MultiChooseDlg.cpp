#include "MultiChooseDlg.h"

#include <QHeaderView>
#include <QCheckBox>
#include <QTableWidget>

MultiChooseDlg::MultiChooseDlg(QWidget *parent, QStringList& itemToChoose)
	: QDialog(parent)
{
  setWindowTitle(tr("Layers"));

	_table = new QTableWidget(this);
	_table->setRowCount(itemToChoose.size());
	_table->setColumnCount(2);
	_table->verticalHeader()->hide();
	_table->horizontalHeader()->hide();
	_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	_table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	_table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

	for (int i = 0; i < itemToChoose.size(); i++)
	{
		QTableWidgetItem* tableItem = new QTableWidgetItem(itemToChoose[i]);
		_table->setItem(i, 0, tableItem);
		_table->setCellWidget(i, 1, new QCheckBox);
	}

	_table->adjustSize();
}

MultiChooseDlg::~MultiChooseDlg()
{

}

QStringList MultiChooseDlg::getCheckedItems()
{
	QStringList checkedItems;

	for (int i = 0; i < _table->rowCount(); i++)
	{
		if (((QCheckBox*)_table->cellWidget(i, 1))->isChecked())
		{
			checkedItems.push_back(_table->item(i, 0)->text());
		}
	}
	return checkedItems;
}
