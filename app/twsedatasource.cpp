#include "twsedatasource.h"
#include "backtesting/historicaldata.h"
#include "csvparser.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <set>

using backtesting::HistoricalData;

namespace {
    const QString DATA_DIR_PATH(QStringLiteral("data/twse"));

    QNetworkAccessManager nam;
}

QString TwseDataSource::dataDirPath()
{
    return QDir::cleanPath(QDir::currentPath() + '/' + DATA_DIR_PATH);
}

QString TwseDataSource::dataFilePath(const QString &productCode, int year, int month)
{
    return QDir::cleanPath(QStringLiteral("%1/%2-%3%4.csv").arg(dataDirPath()).arg(productCode).arg(year, 4).arg(month, 2, 10, QLatin1Char('0')));
}

std::unique_ptr<QNetworkReply> TwseDataSource::downloadHistoricalData(const QString &productCode, int year, int month)
{
    // 產生超連結
    QString dateStr = QStringLiteral("%1%2%3").arg(year).arg(month, 2, 10, QLatin1Char('0')).arg(QLatin1String("01"));
    QString urlStr = QStringLiteral("http://www.twse.com.tw/exchangeReport/STOCK_DAY?response=csv&date=%1&stockNo=%2").arg(dateStr).arg(productCode);
    QUrl url(urlStr);
    // 送出 HTTP request
    QNetworkReply *reply = nam.get(QNetworkRequest(url));
    return std::unique_ptr<QNetworkReply>(reply);
}

std::unique_ptr<backtesting::HistoricalData> TwseDataSource::getHistoricalData(const QString &productCode)
{
    std::unique_ptr<HistoricalData> result = std::make_unique<HistoricalData>();
    // 列出符合商品代碼的檔名
    QDir dataDir(TwseDataSource::dataDirPath());
    QStringList nameFilters(productCode + QStringLiteral("-??????.csv"));
    QFileInfoList entryInfoList = dataDir.entryInfoList(nameFilters, QDir::Files, QDir::Name | QDir::Reversed);
    for (const QFileInfo &fileInfo : entryInfoList) {
        std::unique_ptr<HistoricalData> hData = parseHistoricalData(fileInfo.filePath());
        // 從新到舊新增
        for (int i = hData->length() - 1; i >= 0; --i) {
            result->add(hData->dates[i], hData->opens[i], hData->highs[i], hData->lows[i], hData->closes[i]);
        }
    }
    return result;
}

std::unique_ptr<backtesting::HistoricalData> TwseDataSource::getHistoricalData(const QString &productCode, const QDate &beginDate, const QDate &endDate)
{
    std::unique_ptr<HistoricalData> result = std::make_unique<HistoricalData>();
    // 讀取日期範圍內的檔案
    QDate bDate(beginDate.year(), beginDate.month(), 1);
    QDate date(endDate.year(), endDate.month(), 1);
    while (date >= bDate) {
        QString filePath = dataFilePath(productCode, date.year(), date.month());
        std::unique_ptr<HistoricalData> hData = parseHistoricalData(filePath);
        // 從新到舊新增
        for (int i = hData->length() - 1; i >= 0; --i) {
            // 如果在日期範圍內就新增
            QDate hDataDate = hData->dates[i];
            if (hDataDate >= beginDate && hDataDate <= endDate) {
                result->add(hData->dates[i], hData->opens[i], hData->highs[i], hData->lows[i], hData->closes[i]);
            }
        }
        date = date.addMonths(-1);
    }
    return result;
}

QStringList TwseDataSource::listProductCodes()
{
    // 列出全部資料檔名
    // 使用 std::set 排序和過濾重覆代碼
    QRegularExpression filenameRe(QStringLiteral("^(.+)-.+\\.csv$"));
    std::set<QString> productCodes;
    QDir dataDir(TwseDataSource::dataDirPath());
    QStringList nameFilters(QStringLiteral("\?\?\?\?-\?\?\?\?\?\?.csv"));
    QStringList entryList = dataDir.entryList(nameFilters, QDir::Files);
    for (const QString &entry : entryList) {
        QRegularExpressionMatch match = filenameRe.match(entry);
        if (match.hasMatch()) {
            // 取出商品代碼
            productCodes.emplace(match.captured(1));
        }
    }
    QStringList result;
    for (const QString &code : productCodes) {
        result.append(code);
    }
    return result;
}

std::unique_ptr<backtesting::HistoricalData> TwseDataSource::parseHistoricalData(const QByteArray &data)
{
    std::unique_ptr<HistoricalData> result = std::make_unique<HistoricalData>();
    // Parse CSV
    QRegularExpression dateRe(QStringLiteral("^\\s*(\\d+)/(\\d+)/(\\d+)$"));
    QTextStream stream(data);
    stream.setCodec("Big5");
    QString line;
    while (stream.readLineInto(&line)) {
        QStringList values = CsvParser::parseLine(line);
        // 如果讀到至少 9 個欄位值
        if (values.size() >= 9) {
            // 讀取日期
            QRegularExpressionMatch match = dateRe.match(values[0]);
            if (!match.hasMatch()) {
                continue;
            }
            // 轉成西元年
            bool ok;
            int year = match.captured(1).toInt(&ok) + 1911;
            if (!ok) {
                continue;
            }
            int month = match.captured(2).toInt(&ok);
            if (!ok) {
                continue;
            }
            int day = match.captured(3).toInt(&ok);
            if (!ok) {
                continue;
            }
            QDate date(year, month, day);
            if (!date.isValid()) {
                continue;
            }
            // 讀取開盤價
            double open = values[3].toDouble(&ok);
            if (!ok) {
                continue;
            }
            // 讀取最高價
            double high = values[4].toDouble(&ok);
            if (!ok) {
                continue;
            }
            // 讀取最低價
            double low = values[5].toDouble(&ok);
            if (!ok) {
                continue;
            }
            // 讀取收盤價
            double close = values[6].toDouble(&ok);
            if (!ok) {
                continue;
            }
            result->add(date, open, high, low, close);
        }
    }
    return result;
}

std::unique_ptr<backtesting::HistoricalData> TwseDataSource::parseHistoricalData(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::make_unique<HistoricalData>();
    }
    QByteArray data = file.readAll();
    return parseHistoricalData(data);
}
