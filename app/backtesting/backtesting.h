#ifndef BACKTESTING_BACKTESTING_H
#define BACKTESTING_BACKTESTING_H

#include "historicaldata.h"

namespace backtesting {

class Backtesting
{
public:
    Backtesting(const HistoricalData& hData);

    bool isFinished() const { return currentIndex_ >= hData_.length(); }

    void next() { if (!isFinished()) { ++currentIndex_; } }

private:
    const HistoricalData &hData_;
    int currentIndex_ = 0;
};

} // namespace backtesting

#endif // BACKTESTING_BACKTESTING_H
