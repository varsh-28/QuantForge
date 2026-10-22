#include "Strategy.h"

#include <cstddef>


Strategy::Strategy(int period)
    : period(period)
{
}


/*
 * Simple Moving Average
 *
 * SMA =
 *
 * (Price1 + Price2 + ... + PriceN) / N
 */
double Strategy::calculateSMA(
    const std::vector<PriceBar>& bars) const
{
    if (bars.size() <
        static_cast<std::size_t>(period))
    {
        return 0.0;
    }


    double sum = 0.0;


    const std::size_t start =
        bars.size() -
        static_cast<std::size_t>(period);


    for (
        std::size_t i = start;
        i < bars.size();
        ++i)
    {
        sum += bars[i].close;
    }


    return sum /
        static_cast<double>(period);
}


/*
 * Trading strategy
 *
 * Current price > SMA -> BUY
 *
 * Current price < SMA -> SELL
 *
 * Current price == SMA -> HOLD
 */
Signal Strategy::generateSignal(
    const std::vector<PriceBar>& bars) const
{
    if (bars.size() <
        static_cast<std::size_t>(period))
    {
        return Signal::HOLD;
    }


    const double sma =
        calculateSMA(bars);


    const double currentPrice =
        bars.back().close;


    if (currentPrice > sma)
    {
        return Signal::BUY;
    }


    if (currentPrice < sma)
    {
        return Signal::SELL;
    }


    return Signal::HOLD;
}


/*
 * Convert signal to text.
 */
std::string Strategy::signalToString(
    Signal signal) const
{
    switch (signal)
    {
        case Signal::BUY:
            return "BUY";

        case Signal::SELL:
            return "SELL";

        case Signal::HOLD:
            return "HOLD";
    }


    return "UNKNOWN";
}
