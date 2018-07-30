#include "datamanagementwindow.h"
#include "ui_datamanagementwindow.h"
#include "backtesting/historicaldata.h"
#include "csvparser.h"
#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QTimer>
#include <QNetworkReply>
#include <QMessageBox>
#include "twsedatasource.h"
#include <utility>

using backtesting::HistoricalData;

namespace {
    // 下載間隔時間(ms)
    constexpr int DOWNLOAD_INTERVAL_MS = 5000;
}

DataManagementWindow::DataManagementWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DataManagementWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    // 填入已下載的商品代碼
    ui->productCodeComboBox->addItems(TwseDataSource::listProductCodes());
    ui->productCodeComboBox->setCurrentIndex(-1);
    // 填入 1992 到今天的年份
    QDateTime now = QDateTime::currentDateTime();
    int currentYear = now.date().year();
    for (int year = currentYear; year >= 1992; --year) {
        ui->beginYearComboBox->addItem(QString::number(year));
        ui->endYearComboBox->addItem(QString::number(year));
    }
    // 填入全部月份
    for (int month = 1; month <= 12; ++month) {
        ui->beginMonthComboBox->addItem(QString::number(month));
        ui->endMonthComboBox->addItem(QString::number(month));
    }
    // 填入開始日期
    ui->beginYearComboBox->setCurrentIndex(0);
    int currentMonth = now.date().month();
    ui->beginMonthComboBox->setCurrentIndex(currentMonth - 1);
    // 填入結束日期
    ui->endYearComboBox->setCurrentIndex(0);
    ui->endMonthComboBox->setCurrentIndex(currentMonth - 1);
}

DataManagementWindow::~DataManagementWindow()
{
    delete ui;
}

void DataManagementWindow::on_downloadButton_clicked()
{
    QString productCode = ui->productCodeComboBox->currentText().trimmed();
    if (productCode.isEmpty()) {
        QMessageBox::critical(this, windowTitle(), "請輸入代碼！");
        return;
    }
    productCode_ = productCode;
    ui->productCodeComboBox->setEnabled(false);
    ui->beginYearComboBox->setEnabled(false);
    ui->beginMonthComboBox->setEnabled(false);
    ui->endYearComboBox->setEnabled(false);
    ui->endMonthComboBox->setEnabled(false);
    ui->downloadButton->setEnabled(false);
    // 開始日期
    QDate beginDate = QDate(ui->beginYearComboBox->currentText().toInt(), ui->beginMonthComboBox->currentText().toInt(), 1);
    // 結束日期
    endDate_ = QDate(ui->endYearComboBox->currentText().toInt(), ui->endMonthComboBox->currentText().toInt(), 1);
    // 如果結束日期比較小就調換
    if (endDate_ < beginDate) {
        std::swap(endDate_, beginDate);
    }
    currentDate_ = beginDate;
    ui->logEdit->clear();
    ui->logEdit->appendPlainText(QString("下載日期區間 %1-%2 -> %3-%4").arg(beginDate.year()).arg(beginDate.month(), 2, 10, QLatin1Char('0')).arg(endDate_.year()).arg(endDate_.month(), 2, 10, QLatin1Char('0')));
    // 開始下載
    startToDownload();
}

void DataManagementWindow::on_queryButton_clicked()
{
    QString productCode = ui->productCodeComboBox->currentText().trimmed();
    if (productCode.isEmpty()) {
        QMessageBox::critical(this, windowTitle(), "請輸入代碼！");
        return;
    }
    std::unique_ptr<HistoricalData> hData = TwseDataSource::getHistoricalData(productCode);
    QTableWidget *tw = ui->dataTableWidget;
    tw->setRowCount(0);
    for (int row = 0; row < hData->length(); ++row) {
        tw->insertRow(row);
        // 日期
        QTableWidgetItem *item = new QTableWidgetItem(hData->dates[row].toString(QStringLiteral("yyyy-MM-dd")));
        tw->setItem(row, 0, item);
        item->setTextAlignment(Qt::AlignCenter);
        // 開盤價
        item = new QTableWidgetItem(QString::number(hData->opens[row], 'f', 2));
        tw->setItem(row, 1, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        // 最高價
        item = new QTableWidgetItem(QString::number(hData->highs[row], 'f', 2));
        tw->setItem(row, 2, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        // 最低價
        item = new QTableWidgetItem(QString::number(hData->lows[row], 'f', 2));
        tw->setItem(row, 3, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        // 收盤價
        item = new QTableWidgetItem(QString::number(hData->closes[row], 'f', 2));
        tw->setItem(row, 4, item);
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    }
}

void DataManagementWindow::onReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    QNetworkReply::NetworkError error = reply->error();
    if (QNetworkReply::NoError == error) {
        // 全部讀進來
        QByteArray data = reply->readAll();
        // 取得 Big5 codec
        QTextCodec *codec = QTextCodec::codecForName("Big5");
        if (codec) {
            ui->logEdit->appendPlainText(QStringLiteral("--------------------------------------------------"));
            // 轉碼到 Unicode
            QString dataStr = codec->toUnicode(data);
            ui->logEdit->appendPlainText(dataStr);
            ui->logEdit->appendPlainText(QStringLiteral("--------------------------------------------------"));
            std::unique_ptr<HistoricalData> hData = TwseDataSource::parseHistoricalData(data);
            ui->logEdit->appendPlainText(QString("讀取到 %1 筆資料").arg(hData->length()));
            // 如果有資料
            if (hData->length() > 0) {
                // 儲存檔案
                saveToFile(data);
            }
        } else {
            ui->logEdit->appendPlainText("無法取得 Big5 codec！");
        }
    } else {
        ui->logEdit->appendPlainText("下載發生錯誤！" + reply->errorString());
    }
    // 如果到達結束日期就結束下載，否則開始下載下個月的資料
    if (currentDate_ == endDate_) {
        ui->productCodeComboBox->setEnabled(true);
        ui->beginYearComboBox->setEnabled(true);
        ui->beginMonthComboBox->setEnabled(true);
        ui->endYearComboBox->setEnabled(true);
        ui->endMonthComboBox->setEnabled(true);
        ui->downloadButton->setEnabled(true);
        ui->queryButton->click();
    } else {
        currentDate_ = currentDate_.addMonths(1);
        QTimer::singleShot(DOWNLOAD_INTERVAL_MS, this, &DataManagementWindow::startToDownload);
    }
}

bool DataManagementWindow::saveToFile(const QByteArray &data)
{
    // 決定路徑
    QString dataFilePath = TwseDataSource::dataFilePath(productCode_, currentDate_.year(), currentDate_.month());
    ui->logEdit->appendPlainText("寫入 " + QDir::toNativeSeparators(dataFilePath));
    // 建立目錄
    QDir dataDir(TwseDataSource::dataDirPath());
    if (!dataDir.mkpath(dataDir.path())) {
        QMessageBox::critical(this, windowTitle(), "建立目錄發生錯誤！");
        return false;
    }
    // 建立檔案
    QFile file(dataFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, windowTitle(), "建立檔案發生錯誤！");
        return false;
    }
    // 寫入檔案
    if (-1 == file.write(data)) {
        QMessageBox::critical(this, windowTitle(), "寫入檔案發生錯誤！");
        return false;
    }
    return true;
}

void DataManagementWindow::startToDownload()
{
    std::unique_ptr<QNetworkReply> reply = TwseDataSource::downloadHistoricalData(productCode_, currentDate_.year(), currentDate_.month());
    ui->logEdit->appendPlainText("下載 " + reply->request().url().toString());
    connect(reply.get(), &QNetworkReply::finished,
            this, &DataManagementWindow::onReplyFinished);
    reply.release();
}
