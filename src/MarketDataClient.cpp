#include "MarketDataClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace
{

size_t writeCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userData)
{
    const size_t totalSize = size * nmemb;

    auto* response = static_cast<std::string*>(userData);

    response->append(
        static_cast<char*>(contents),
        totalSize
    );

    return totalSize;
}


std::string formatDate(
    const std::chrono::system_clock::time_point& point)
{
    const std::time_t time =
        std::chrono::system_clock::to_time_t(point);

    std::tm utcTime{};

#ifdef _WIN32
    gmtime_s(&utcTime, &time);
#else
    gmtime_r(&time, &utcTime);
#endif

    std::ostringstream output;
    output << std::put_time(&utcTime, "%Y-%m-%d");

    return output.str();
}


std::string urlEncode(
    CURL* curl,
    const std::string& value)
{
    char* encoded = curl_easy_escape(
        curl,
        value.c_str(),
        static_cast<int>(value.length())
    );

    if (encoded == nullptr)
    {
        throw std::runtime_error(
            "Failed to encode URL parameter."
        );
    }

    std::string result(encoded);
    curl_free(encoded);

    return result;
}

} // namespace


MarketDataClient::MarketDataClient(
    const std::string& apiKey,
    const std::string& secretKey)
    : apiKey(apiKey),
      secretKey(secretKey)
{
}


std::vector<PriceBar> MarketDataClient::getHistoricalBars(
    const std::string& symbol,
    int limit)
{
    if (symbol.empty())
    {
        throw std::invalid_argument(
            "Stock symbol cannot be empty."
        );
    }

    if (limit <= 0)
    {
        throw std::invalid_argument(
            "Bar limit must be greater than zero."
        );
    }

    if (apiKey.empty() || secretKey.empty())
    {
        throw std::runtime_error(
            "Missing Alpaca API credentials. Set "
            "APCA_API_KEY_ID and APCA_API_SECRET_KEY."
        );
    }

    CURL* curl = curl_easy_init();

    if (curl == nullptr)
    {
        throw std::runtime_error(
            "Could not initialize libcurl."
        );
    }

    std::vector<PriceBar> bars;
    std::string response;
    struct curl_slist* headers = nullptr;

    try
    {
        /*
         * Request a sufficiently large calendar window.
         * Sort descending so the API returns the newest bars
         * first when the requested limit is reached.
         */
        const auto now =
            std::chrono::system_clock::now();

        const auto endPoint =
            now - std::chrono::hours(24);

        const auto startPoint =
            endPoint -
            std::chrono::hours(24LL * (limit * 3LL + 10LL));

        const std::string startDate =
            formatDate(startPoint);

        const std::string endDate =
            formatDate(endPoint);

        const std::string url =
            "https://data.alpaca.markets/v2/stocks/" +
            urlEncode(curl, symbol) +
            "/bars?timeframe=1Day" +
            "&start=" +
            urlEncode(curl, startDate + "T00:00:00Z") +
            "&end=" +
            urlEncode(curl, endDate + "T23:59:59Z") +
            "&limit=" + std::to_string(limit) +
            "&feed=iex&sort=desc";

        std::cerr
            << "\n[DEBUG] Market Data URL:\n"
            << url << '\n'
            << "[DEBUG] Start Date: "
            << startDate << '\n'
            << "[DEBUG] End Date: "
            << endDate << '\n';

        const std::string keyHeader =
            "APCA-API-KEY-ID: " + apiKey;

        const std::string secretHeader =
            "APCA-API-SECRET-KEY: " + secretKey;

        headers = curl_slist_append(
            headers,
            keyHeader.c_str()
        );

        if (headers == nullptr)
        {
            throw std::runtime_error(
                "Failed to create API key header."
            );
        }

        struct curl_slist* updatedHeaders =
            curl_slist_append(
                headers,
                secretHeader.c_str()
            );

        if (updatedHeaders == nullptr)
        {
            throw std::runtime_error(
                "Failed to create API secret header."
            );
        }

        headers = updatedHeaders;

        updatedHeaders = curl_slist_append(
            headers,
            "Accept: application/json"
        );

        if (updatedHeaders == nullptr)
        {
            throw std::runtime_error(
                "Failed to create Accept header."
            );
        }

        headers = updatedHeaders;

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

        curl_easy_setopt(
            curl,
            CURLOPT_CONNECTTIMEOUT,
            10L
        );

        curl_easy_setopt(
            curl,
            CURLOPT_TIMEOUT,
            30L
        );

        curl_easy_setopt(
            curl,
            CURLOPT_SSL_VERIFYPEER,
            1L
        );

        curl_easy_setopt(
            curl,
            CURLOPT_SSL_VERIFYHOST,
            2L
        );

        const CURLcode result =
            curl_easy_perform(curl);

        long httpStatus = 0;

        curl_easy_getinfo(
            curl,
            CURLINFO_RESPONSE_CODE,
            &httpStatus
        );

        if (result != CURLE_OK)
        {
            throw std::runtime_error(
                std::string("Market-data request failed: ") +
                curl_easy_strerror(result)
            );
        }

        if (httpStatus < 200 || httpStatus >= 300)
        {
            throw std::runtime_error(
                "Alpaca Market Data API returned HTTP " +
                std::to_string(httpStatus) +
                "\nResponse: " + response
            );
        }

        const json data = json::parse(response);

        if (!data.contains("bars") ||
            data["bars"].is_null() ||
            !data["bars"].is_array())
        {
            throw std::runtime_error(
                "Alpaca returned no historical bars for " +
                symbol + ". Response: " + response
            );
        }

        for (const auto& item : data["bars"])
        {
            PriceBar bar;

            bar.timestamp =
                item.at("t").get<std::string>();

            bar.open =
                item.at("o").get<double>();

            bar.high =
                item.at("h").get<double>();

            bar.low =
                item.at("l").get<double>();

            bar.close =
                item.at("c").get<double>();

            bar.volume =
                item.at("v").get<long long>();

            bars.push_back(bar);
        }

        /*
         * The API response is descending. Reverse the order
         * so bars.back() represents the newest returned bar.
         */
        std::reverse(bars.begin(), bars.end());

        std::cerr
            << "[DEBUG] Bars received: "
            << bars.size() << '\n';

        if (!bars.empty())
        {
            std::cerr
                << "[DEBUG] First bar: "
                << bars.front().timestamp << '\n'
                << "[DEBUG] Last bar:  "
                << bars.back().timestamp << '\n';
        }

        curl_slist_free_all(headers);
        headers = nullptr;

        curl_easy_cleanup(curl);
        curl = nullptr;
    }
    catch (...)
    {
        if (headers != nullptr)
        {
            curl_slist_free_all(headers);
        }

        if (curl != nullptr)
        {
            curl_easy_cleanup(curl);
        }

        throw;
    }

    return bars;
}
