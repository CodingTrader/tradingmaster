#ifndef TWSEDATASOURCE_H
#define TWSEDATASOURCE_H

#include <memory>
#include <QtCore/QDate>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QByteArray;
class QNetworkReply;
namespace backtesting {
class HistoricalData;
}

class TwseDataSource
{
public:
    static QString dataDirPath();
    static QString dataFilePath(const QString &productCode, int year, int month);

    static std::unique_ptr<QNetworkReply> downloadHistoricalData(const QString &productCode, int year, int month);
    static std::unique_ptr<backtesting::HistoricalData> getHistoricalData(const QString &productCode);
    static std::unique_ptr<backtesting::HistoricalData> getHistoricalData(const QString &productCode, const QDate &beginDate, const QDate &endDate);
    static QStringList listProductCodes();
    static std::unique_ptr<backtesting::HistoricalData> parseHistoricalData(const QByteArray &data);
    static std::unique_ptr<backtesting::HistoricalData> parseHistoricalData(const QString &filePath);
};

#endif // TWSEDATASOURCE_H
