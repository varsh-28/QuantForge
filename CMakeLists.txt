cmake_minimum_required(VERSION 3.16)

project(AlgoTradeEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(AlgoTradeEngine

    src/main.cpp

    src/MarketDataClient.cpp
    src/Strategy.cpp
    src/TradingClient.cpp

)

target_include_directories(
    AlgoTradeEngine
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    AlgoTradeEngine
    PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
)
