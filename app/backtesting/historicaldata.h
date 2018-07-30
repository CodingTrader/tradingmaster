#ifndef BACKTESTING_HISTORICALDATA_H
#define BACKTESTING_HISTORICALDATA_H

#include <QtCore/QDate>
#include <QtCore/QVector>

namespace backtesting {

// 歷史資料
class HistoricalData
{
public:
    // 日期
    QVector<QDate> dates;
    // 開盤價
    QVector<double> opens;
    // 最高價
    QVector<double> highs;
    // 最低價
    QVector<double> lows;
    // 收盤價
    QVector<double> closes;

    // 新增資料
    void add(const QDate &date, double open, double high, double low, double close)
    {
        dates.append(date);
        opens.append(open);
        highs.append(high);
        lows.append(low);
        closes.append(close);
    }

    // 資料筆數
    int length() const { return dates.length(); }
};

} // namespace backtesting

#endif // BACKTESTING_HISTORICALDATA_H
