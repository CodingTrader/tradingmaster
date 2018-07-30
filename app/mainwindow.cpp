#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "backtesting/backtesting.h"
#include "backtesting/historicaldata.h"
#include "backtesting/strategy.h"
#include "datamanagementwindow.h"
#include "examplestrategy1.h"
#include <numeric>
#include <QBarCategoryAxis>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QChart>
#include <QChartView>
#include <QDate>
#include <QDateTime>
#include <QMessageBox>
#include <QStringList>
#include <QValueAxis>
#include "twsedatasource.h"

using backtesting::Backtesting;
using backtesting::HistoricalData;
using backtesting::Strategy;
using QtCharts::QBarCategoryAxis;
using QtCharts::QCandlestickSeries;
using QtCharts::QCandlestickSet;
using QtCharts::QChart;
using QtCharts::QChartView;
using QtCharts::QValueAxis;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    reloadProductCodes();
    // 填入開始日期
    ui->beginDateEdit->setDate(QDate::currentDate().addYears(-1).addDays(1));
    // 填入結束日期
    ui->endDateEdit->setDate(QDate::currentDate());
    // QChartView
    chartView_ = new QChartView(ui->dataGroupBox);
    chartView_->setRenderHint(QPainter::Antialiasing);
    ui->dataGroupBoxLayout->addWidget(chartView_);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::calculateSMA(int n, int column)
{
    if (n <= 0) {
        return;
    }
    double result = 0.0;
    QTableWidget *tw = ui->dataTableWidget;
    for (int row = hData_->length() - 1; row >= 0; --row) {
        // 計算可用資料長度
        int usableLength = hData_->length() - row;
        QTableWidgetItem *item;
        // 如果資料夠多
        if (usableLength >= n) {
            // 如果可用資料長度為 n 就計算第一個結果
            if (usableLength == n) {
                result = std::accumulate(hData_->closes.data() + row, hData_->closes.data() + row + n, 0.0) / n;
            }
            // 否則利用之前算出的結果計算出新結果
            else {
                result = result - hData_->closes[row + n] / n + hData_->closes[row] / n;
            }
            item = new QTableWidgetItem(QString::number(result, 'f', 2));
        }
        // 如果資料不夠多
        else {
            item = new QTableWidgetItem(QStringLiteral("--"));
        }
        tw->setItem(row, column, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    }
}

void MainWindow::on_dataManagementAction_triggered()
{
    DataManagementWindow *window = new DataManagementWindow(this);
    window->setWindowModality(Qt::ApplicationModal);
    connect(window, &DataManagementWindow::destroyed,
            this, [this]() {
        reloadProductCodes();
        if (ui->productCodeComboBox->currentIndex() != -1) {
            ui->loadDataButton->click();
        }
    });
    window->show();
}

void MainWindow::on_loadDataButton_clicked()
{
    if (-1 == ui->productCodeComboBox->currentIndex()) {
        QMessageBox::critical(this, windowTitle(), "請選擇代碼！");
        return;
    }
    QString productCode = ui->productCodeComboBox->currentText();
    hData_ = TwseDataSource::getHistoricalData(productCode, ui->beginDateEdit->date(), ui->endDateEdit->date());
    QTableWidget *tw = ui->dataTableWidget;
    tw->setRowCount(0);
    for (int row = 0; row < hData_->length(); ++row) {
        tw->insertRow(row);
        // 日期
        QTableWidgetItem *item = new QTableWidgetItem(hData_->dates[row].toString(QStringLiteral("yyyy-MM-dd")));
        tw->setItem(row, 0, item);
        item->setTextAlignment(Qt::AlignCenter);
        // 開盤價
        item = new QTableWidgetItem(QString::number(hData_->opens[row], 'f', 2));
        tw->setItem(row, 1, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        // 最高價
        item = new QTableWidgetItem(QString::number(hData_->highs[row], 'f', 2));
        tw->setItem(row, 2, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        // 最低價
        item = new QTableWidgetItem(QString::number(hData_->lows[row], 'f', 2));
        tw->setItem(row, 3, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        // 收盤價
        item = new QTableWidgetItem(QString::number(hData_->closes[row], 'f', 2));
        tw->setItem(row, 4, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    }
    // 計算技術指標
    // SMA5
    calculateSMA(5, 5);
    // SMA10
    calculateSMA(10, 6);
    // SMA20
    calculateSMA(20, 7);
    // SMA60
    calculateSMA(60, 8);
    // SMA120
    calculateSMA(120, 9);
    // 繪圖
    QCandlestickSeries *series = new QCandlestickSeries;
    series->setName(productCode);
    series->setIncreasingColor(QColor(Qt::red));
    series->setDecreasingColor(QColor(Qt::green));
    QStringList categories;
    for (int i = hData_->length() - 1; i >= 0; --i) {
        qint64 msecs = QDateTime(hData_->dates[i]).toMSecsSinceEpoch();
        QCandlestickSet *candlestickSet = new QCandlestickSet(hData_->opens[i], hData_->highs[i], hData_->lows[i], hData_->closes[i], msecs);
        series->append(candlestickSet);
        categories << QDateTime::fromMSecsSinceEpoch(msecs).toString(QStringLiteral("yyyy-MM-dd"));
    }
    QChart *chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QString("%1 - %2").arg(ui->beginDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"))).arg(ui->endDateEdit->date().toString(QStringLiteral("yyyy-MM-dd"))));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->createDefaultAxes();
    QBarCategoryAxis *axisX = qobject_cast<QBarCategoryAxis *>(chart->axes(Qt::Horizontal).at(0));
    axisX->setCategories(categories);
    QValueAxis *axisY = qobject_cast<QValueAxis *>(chart->axes(Qt::Vertical).at(0));
    axisY->setMax(axisY->max() * 1.01);
    axisY->setMin(axisY->min() * 0.99);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chartView_->setChart(chart);
}

void MainWindow::on_startButton_clicked()
{
    // TODO
    ExampleStrategy1 strategy;
    Backtesting bt(*hData_);
    while (!bt.isFinished()) {
        strategy.apply(bt);
        bt.next();
    }
}

void MainWindow::reloadProductCodes()
{
    // 保存目前選擇的商品代碼
    QString productCode = ui->productCodeComboBox->currentText();
    ui->productCodeComboBox->clear();
    // 填入已下載的商品代碼
    ui->productCodeComboBox->addItems(TwseDataSource::listProductCodes());
    // 選擇之前選的商品代碼
    ui->productCodeComboBox->setCurrentIndex(ui->productCodeComboBox->findText(productCode));
}
