#pragma once

#include <string>
#include <vector>

#include "MarketDataClient.h"


enum class Signal
{
    BUY,
    SELL,
    HOLD
};


class Strategy
{
public:

    explicit Strategy(int period = 20);


    /*
     * Calculate Simple Moving Average.
     */
    double calculateSMA(
        const std::vector<PriceBar>& bars
    ) const;


    /*
     * Generate trading signal.
     */
    Signal generateSignal(
        const std::vector<PriceBar>& bars
    ) const;


    /*
     * Convert enum to readable text.
     */
    std::string signalToString(
        Signal signal
    ) const;


private:

    int period;
};
