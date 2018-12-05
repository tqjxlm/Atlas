#ifndef TILESELECTDIALOG_H
#define TILESELECTDIALOG_H

#include <QWidget>

namespace Ui {
    class TileSelectDialog;
}

class TileSelectDialog : public QWidget
{
    Q_OBJECT

public:
    TileSelectDialog(int itemTotalCount, QWidget *parent = 0);
    ~TileSelectDialog();

public slots:
    void selectTileSlot(const QString& tileName);
    void unselectTileSlot(const QString& tileName);
    void okAllBtClicked();
    void cancleAllBtClicked();
    void updateLabelCount();

signals:
    void selectAllTile();
    void unselectAllTile();
    void closed();

protected:
    virtual void closeEvent(QCloseEvent *e) override;

private:
    Ui::TileSelectDialog* _ui;
    int _itemTotalCount;
};

#endif // TILESELECTDIALOG_H
