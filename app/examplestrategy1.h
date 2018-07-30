#ifndef EXAMPLESTRATEGY1_H
#define EXAMPLESTRATEGY1_H

#include "backtesting/strategy.h"

class ExampleStrategy1 : public backtesting::Strategy
{
public:
    virtual void apply(const backtesting::Backtesting &bt) override;
};

#endif // EXAMPLESTRATEGY1_H
