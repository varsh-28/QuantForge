#pragma once

#include <string>


struct AccountInfo
{
    double buyingPower = 0.0;
    bool tradingBlocked = false;
};


struct PositionInfo
{
    bool exists = false;

    double quantity = 0.0;
    double marketValue = 0.0;
    double averageEntryPrice = 0.0;
};


class TradingClient
{
public:

    TradingClient(
        const std::string& apiKey,
        const std::string& secretKey
    );


    /*
     * Retrieve paper-account information.
     */
    AccountInfo getAccount();


    /*
     * Check whether a position exists.
     */
    PositionInfo getPosition(
        const std::string& symbol
    );


    /*
     * Submit paper market order.
     */
    std::string submitMarketOrder(
        const std::string& symbol,
        const std::string& side,
        double quantity
    );


private:

    std::string apiKey;
    std::string secretKey;


    std::string request(
        const std::string& method,
        const std::string& url,
        const std::string& body = ""
    );
};
