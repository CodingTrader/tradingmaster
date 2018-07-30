#ifndef DATAMANAGEMENTWINDOW_H
#define DATAMANAGEMENTWINDOW_H

#include <QtCore/QDate>
#include <QtCore/QString>
#include <QtWidgets/QMainWindow>

class QByteArray;
namespace Ui {
class DataManagementWindow;
}

class DataManagementWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DataManagementWindow(QWidget *parent = nullptr);
    ~DataManagementWindow();

private slots:
    void on_downloadButton_clicked();
    void on_queryButton_clicked();
    void onReplyFinished();

private:
    bool saveToFile(const QByteArray &data);
    void startToDownload();

    Ui::DataManagementWindow *ui;
    // 正在下載的商品代碼
    QString productCode_;
    // 下載的結束日期
    QDate endDate_;
    // 正在下載的日期
    QDate currentDate_;
};

#endif // DATAMANAGEMENTWINDOW_H
