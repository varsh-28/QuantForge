#include "MarketDataClient.h"
#include "Strategy.h"
#include "TradingClient.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


/*
 * Remove whitespace from a string.
 */
std::string trim(
    const std::string& input)
{
    const std::string whitespace =
        " \t\n\r";


    const std::size_t first =
        input.find_first_not_of(whitespace);


    if (first == std::string::npos)
    {
        return "";
    }


    const std::size_t last =
        input.find_last_not_of(whitespace);


    return input.substr(
        first,
        last - first + 1
    );
}


/*
 * Convert:
 *
 * AAPL, MSFT, NVDA
 *
 * into:
 *
 * ["AAPL", "MSFT", "NVDA"]
 */
std::vector<std::string>
parseSymbols(
    const std::string& input)
{
    std::vector<std::string> symbols;

    std::stringstream stream(input);

    std::string symbol;


    while (
        std::getline(
            stream,
            symbol,
            ','))
    {
        symbol = trim(symbol);


        if (!symbol.empty())
        {
            std::transform(
                symbol.begin(),
                symbol.end(),
                symbol.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(
                        std::toupper(c)
                    );
                }
            );


            symbols.push_back(symbol);
        }
    }


    return symbols;
}


/*
 * Read an environment variable.
 */
std::string getEnvironmentVariable(
    const char* name)
{
    const char* value =
        std::getenv(name);


    if (value == nullptr)
    {
        return "";
    }


    return value;
}


/*
 * Print money.
 */
void printMoney(double value)
{
    std::cout
        << "$"
        << std::fixed
        << std::setprecision(2)
        << value;
}


int main()
{
    try
    {
        std::cout
            << "\n========================================\n"
            << "       C++ ALGO TRADE ENGINE\n"
            << "       REAL DATA / PAPER TRADING\n"
            << "========================================\n\n";


        /*
         * API credentials come from environment variables.
         *
         * We DO NOT hard-code them in source code.
         */
        const std::string apiKey =
            getEnvironmentVariable(
                "APCA_API_KEY_ID"
            );


        const std::string secretKey =
            getEnvironmentVariable(
                "APCA_API_SECRET_KEY"
            );


        if (
            apiKey.empty() ||
            secretKey.empty())
        {
            std::cerr
                << "ERROR: Alpaca credentials are missing.\n\n"

                << "Set them with:\n\n"

                << "export APCA_API_KEY_ID=\"YOUR_KEY\"\n"
                << "export APCA_API_SECRET_KEY=\"YOUR_SECRET\"\n";

            return 1;
        }


        /*
         * Get stock symbols.
         */
        std::cout
            << "Enter stock symbols separated by commas.\n"
            << "Example: AAPL,MSFT,NVDA\n\n"
            << "Symbols: ";


        std::string input;

        std::getline(
            std::cin,
            input
        );


        const auto symbols =
            parseSymbols(input);


        if (symbols.empty())
        {
            std::cerr
                << "No valid symbols entered.\n";

            return 1;
        }


        /*
         * Ask whether paper execution is enabled.
         *
         * Default = NO.
         */
        std::cout
            << "\nEnable paper-order execution? (y/n): ";


        std::string executionChoice;

        std::getline(
            std::cin,
            executionChoice
        );


        const bool paperTradingEnabled =
            !executionChoice.empty() &&
            (
                executionChoice[0] == 'y' ||
                executionChoice[0] == 'Y'
            );


        std::cout
            << "\nPaper order execution: "
            << (
                paperTradingEnabled
                    ? "ENABLED"
                    : "DISABLED"
            )
            << "\n";


        /*
         * Create clients.
         */
        MarketDataClient marketData(
            apiKey,
            secretKey
        );


        Strategy strategy(20);


        TradingClient trading(
            apiKey,
            secretKey
        );


        /*
         * If paper trading is enabled,
         * retrieve account information.
         */
        if (paperTradingEnabled)
        {
            const AccountInfo account =
                trading.getAccount();


            std::cout
                << "\nPaper Account\n"
                << "-------------\n"
                << "Buying Power : ";


            printMoney(
                account.buyingPower
            );


            std::cout << "\n";


            if (account.tradingBlocked)
            {
                std::cout
                    << "WARNING: Trading is blocked.\n";
            }
        }


        /*
         * Maximum percentage of buying power
         * allocated to one new position.
         */
        constexpr double MAX_ALLOCATION = 0.10;


        for (
            const std::string& symbol :
            symbols)
        {
            std::cout
                << "\n----------------------------------------\n"
                << "Analyzing "
                << symbol
                << "\n"
                << "----------------------------------------\n";


            try
            {
                /*
                 * Retrieve 30 daily bars.
                 *
                 * We need 20 bars for the SMA,
                 * while the extra bars give us
                 * some historical context.
                 */
                const auto bars =
                    marketData.getHistoricalBars(
                        symbol,
                        30
                    );


                if (bars.size() < 20)
                {
                    std::cout
                        << "Not enough market data for "
                        << symbol
                        << ".\n";

                    continue;
                }


                const PriceBar& latest =
                    bars.back();


                const double sma =
                    strategy.calculateSMA(
                        bars
                    );


                const Signal signal =
                    strategy.generateSignal(
                        bars
                    );


                std::cout
                    << "\nLatest Daily Bar\n"

                    << "Timestamp : "
                    << latest.timestamp
                    << "\n"

                    << "Open      : ";

                printMoney(latest.open);

                std::cout
                    << "\nHigh      : ";

                printMoney(latest.high);

                std::cout
                    << "\nLow       : ";

                printMoney(latest.low);

                std::cout
                    << "\nClose     : ";

                printMoney(latest.close);

                std::cout
                    << "\nVolume    : "
                    << latest.volume

                    << "\n20-Day SMA: ";

                printMoney(sma);

                std::cout
                    << "\nSignal    : "
                    << strategy.signalToString(signal)
                    << "\n";


                /*
                 * Paper execution.
                 */
                if (!paperTradingEnabled)
                {
                    std::cout
                        << "\nPaper trading disabled.\n";

                    continue;
                }


                /*
                 * Get existing position.
                 */
                const PositionInfo position =
                    trading.getPosition(
                        symbol
                    );


                /*
                 * BUY logic
                 */
                if (signal == Signal::BUY)
                {
                    if (position.exists)
                    {
                        std::cout
                            << "\nBUY signal ignored.\n"
                            << "Existing position: "
                            << position.quantity
                            << " shares.\n";

                        continue;
                    }


                    /*
                     * Retrieve latest account
                     * buying power.
                     */
                    const AccountInfo account =
                        trading.getAccount();


                    const double allocation =
                        account.buyingPower *
                        MAX_ALLOCATION;


                    const double quantity =
                        allocation /
                        latest.close;


                    /*
                     * Require at least 0.001 share.
                     */
                    if (quantity < 0.001)
                    {
                        std::cout
                            << "\nInsufficient buying power.\n";

                        continue;
                    }


                    std::cout
                        << "\nBUY order planned\n"
                        << "Allocation: ";


                    printMoney(allocation);


                    std::cout
                        << "\nEstimated quantity: "
                        << quantity
                        << "\n";


                    const std::string orderId =
                        trading.submitMarketOrder(
                            symbol,
                            "buy",
                            quantity
                        );


                    std::cout
                        << "Paper BUY submitted.\n"
                        << "Order ID: "
                        << orderId
                        << "\n";
                }


                /*
                 * SELL logic
                 */
                else if (
                    signal == Signal::SELL)
                {
                    if (!position.exists)
                    {
                        std::cout
                            << "\nSELL signal generated,\n"
                            << "but no existing position "
                            << "was found.\n";

                        continue;
                    }


                    std::cout
                        << "\nSELL signal.\n"
                        << "Existing position: "
                        << position.quantity
                        << " shares.\n";


                    const std::string orderId =
                        trading.submitMarketOrder(
                            symbol,
                            "sell",
                            position.quantity
                        );


                    std::cout
                        << "Paper SELL submitted.\n"
                        << "Order ID: "
                        << orderId
                        << "\n";
                }


                /*
                 * HOLD logic
                 */
                else
                {
                    std::cout
                        << "\nNo order generated.\n";
                }
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "\nERROR processing "
                    << symbol
                    << ":\n"
                    << e.what()
                    << "\n";
            }
        }


        std::cout
            << "\n========================================\n"
            << "              RUN COMPLETE\n"
            << "========================================\n";


        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "\nFATAL ERROR:\n"
            << e.what()
            << "\n";

        return 1;
    }
}
