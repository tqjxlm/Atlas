#ifndef MULTICHOOSEDLG_H
#define MULTICHOOSEDLG_H

#include <QDialog>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class MultiChooseDlg : public QDialog
{
	Q_OBJECT

public:
	MultiChooseDlg(QWidget *parent, QStringList& itemToChoose);
	~MultiChooseDlg();
	QStringList getCheckedItems();

private:
	QTableWidget* _table;
};

#endif // MULTICHOOSEDLG_H
