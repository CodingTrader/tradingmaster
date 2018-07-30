#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QtWidgets/QMainWindow>

namespace backtesting {
class HistoricalData;
}
namespace QtCharts {
class QChartView;
}
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_dataManagementAction_triggered();
    void on_loadDataButton_clicked();
    void on_startButton_clicked();

private:
    void calculateSMA(int n, int column);
    void reloadProductCodes();

    Ui::MainWindow *ui;
    std::unique_ptr<backtesting::HistoricalData> hData_;
    QtCharts::QChartView *chartView_;
};

#endif // MAINWINDOW_H
