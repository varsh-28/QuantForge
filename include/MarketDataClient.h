#pragma once

#include <string>
#include <vector>

/*
 * One OHLCV market-data candle.
 *
 * O = Open
 * H = High
 * L = Low
 * C = Close
 * V = Volume
 */
struct PriceBar
{
    std::string timestamp;

    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;

    long long volume = 0;
};


class MarketDataClient
{
public:

    MarketDataClient(
        const std::string& apiKey,
        const std::string& secretKey
    );

    /*
     * Download historical daily bars.
     *
     * Example:
     *
     * AAPL -> last 30 daily candles
     */
    std::vector<PriceBar> getHistoricalBars(
        const std::string& symbol,
        int limit = 30
    );

private:

    std::string apiKey;
    std::string secretKey;
};
