#ifndef BACKTESTING_STRATEGY_H
#define BACKTESTING_STRATEGY_H

namespace backtesting {

class Backtesting;

class Strategy
{
public:
    virtual ~Strategy() = default;

    virtual void apply(const Backtesting &bt) = 0;
};

} // namespace backtesting

#endif // BACKTESTING_STRATEGY_H
