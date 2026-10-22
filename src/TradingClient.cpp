#include "TradingClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;


namespace
{

size_t writeCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userData)
{
    const size_t totalSize =
        size * nmemb;


    auto* response =
        static_cast<std::string*>(userData);


    response->append(
        static_cast<char*>(contents),
        totalSize
    );


    return totalSize;
}

}


/*
 * Constructor
 */
TradingClient::TradingClient(
    const std::string& apiKey,
    const std::string& secretKey)
    : apiKey(apiKey),
      secretKey(secretKey)
{
}


/*
 * Generic HTTP helper.
 */
std::string TradingClient::request(
    const std::string& method,
    const std::string& url,
    const std::string& body)
{
    CURL* curl =
        curl_easy_init();


    if (curl == nullptr)
    {
        throw std::runtime_error(
            "Could not initialize libcurl."
        );
    }


    std::string response;


    struct curl_slist* headers = nullptr;


    const std::string keyHeader =
        "APCA-API-KEY-ID: " + apiKey;


    const std::string secretHeader =
        "APCA-API-SECRET-KEY: " + secretKey;


    headers = curl_slist_append(
        headers,
        keyHeader.c_str()
    );


    headers = curl_slist_append(
        headers,
        secretHeader.c_str()
    );


    headers = curl_slist_append(
        headers,
        "Accept: application/json"
    );


    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );


    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );


    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );


    if (method == "POST")
    {
        curl_easy_setopt(
            curl,
            CURLOPT_POST,
            1L
        );

        curl_easy_setopt(
            curl,
            CURLOPT_POSTFIELDS,
            body.c_str()
        );
    }


    CURLcode result =
        curl_easy_perform(curl);


    long httpStatus = 0;


    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpStatus
    );


    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);


    if (result != CURLE_OK)
    {
        throw std::runtime_error(
            std::string("Trading API request failed: ") +
            curl_easy_strerror(result)
        );
    }


    if (httpStatus < 200 ||
        httpStatus >= 300)
    {
        throw std::runtime_error(
            "Trading API returned HTTP " +
            std::to_string(httpStatus) +
            "\nResponse: " +
            response
        );
    }


    return response;
}


/*
 * Get paper account.
 */
AccountInfo TradingClient::getAccount()
{
    const std::string response =
        request(
            "GET",
            "https://paper-api.alpaca.markets/v2/account"
        );


    const json data =
        json::parse(response);


    AccountInfo account;


    account.buyingPower =
        std::stod(
            data["buying_power"].get<std::string>()
        );


    account.tradingBlocked =
        data["trading_blocked"].get<bool>();


    return account;
}


/*
 * Get current position.
 */
PositionInfo TradingClient::getPosition(
    const std::string& symbol)
{
    const std::string url =
        "https://paper-api.alpaca.markets/v2/positions/" +
        symbol;


    CURL* curl =
        curl_easy_init();


    if (curl == nullptr)
    {
        throw std::runtime_error(
            "Could not initialize libcurl."
        );
    }


    std::string response;


    struct curl_slist* headers = nullptr;


    const std::string keyHeader =
        "APCA-API-KEY-ID: " + apiKey;


    const std::string secretHeader =
        "APCA-API-SECRET-KEY: " + secretKey;


    headers = curl_slist_append(
        headers,
        keyHeader.c_str()
    );


    headers = curl_slist_append(
        headers,
        secretHeader.c_str()
    );


    headers = curl_slist_append(
        headers,
        "Accept: application/json"
    );


    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );


    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback
    );


    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &response
    );


    CURLcode result =
        curl_easy_perform(curl);


    long httpStatus = 0;


    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpStatus
    );


    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);


    if (result != CURLE_OK)
    {
        throw std::runtime_error(
            std::string("Position request failed: ") +
            curl_easy_strerror(result)
        );
    }


    /*
     * 404 means no open position.
     */
    if (httpStatus == 404)
    {
        return PositionInfo{};
    }


    if (httpStatus != 200)
    {
        throw std::runtime_error(
            "Position API returned HTTP " +
            std::to_string(httpStatus) +
            "\nResponse: " +
            response
        );
    }


    const json data =
        json::parse(response);


    PositionInfo position;


    position.exists = true;


    position.quantity =
        std::stod(
            data["qty"].get<std::string>()
        );


    position.marketValue =
        std::stod(
            data["market_value"].get<std::string>()
        );


    position.averageEntryPrice =
        std::stod(
            data["avg_entry_price"].get<std::string>()
        );


    return position;
}


/*
 * Submit paper market order.
 *
 * Alpaca paper trading endpoint:
 *
 * https://paper-api.alpaca.markets/v2/orders
 */
std::string TradingClient::submitMarketOrder(
    const std::string& symbol,
    const std::string& side,
    double quantity)
{
    std::ostringstream qtyStream;

    qtyStream
        << std::fixed
        << std::setprecision(4)
        << quantity;


    json order;


    order["symbol"] =
        symbol;


    order["qty"] =
        qtyStream.str();


    order["side"] =
        side;


    order["type"] =
        "market";


    order["time_in_force"] =
        "day";


    const std::string response =
        request(
            "POST",
            "https://paper-api.alpaca.markets/v2/orders",
            order.dump()
        );


    const json data =
        json::parse(response);


    if (data.contains("id"))
    {
        return data["id"].get<std::string>();
    }


    return "UNKNOWN";
}
