cmake_minimum_required(VERSION 4.1.0)

project(settei_man LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

add_library(ai SHARED ai/deepl.cpp ai/context.cpp)
target_include_directories(ai PUBLIC ai/)
target_link_libraries(ai PUBLIC nlohmann_json::nlohmann_json CURL::libcurl)

add_executable(SetteiMan)
target_sources(SetteiMan
  PRIVATE
    settei_man/settei_man.cpp
    settei_man/company.cpp
    settei_man/episode.cpp
    settei_man/materials.cpp
    settei_man/cuts.cpp
)
target_include_directories(SetteiMan PRIVATE settei_man ${Boost_INCLUDE_DIRS})
target_link_libraries(SetteiMan ${Boost_LIBRARIES} spdlog::spdlog ai)
