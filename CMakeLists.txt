cmake_minimum_required(VERSION 4.1.0)

project(SetteiMan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_library(APIs)
target_sources(APIs PRIVATE
    apis/ai.cpp
    apis/deepl.cpp
    apis/openrouter.cpp
    apis/google.cpp
)
target_include_directories(APIs PUBLIC apis/ settei_man/)
target_link_libraries(APIs PUBLIC nlohmann_json::nlohmann_json CURL::libcurl)

add_library(SetteiMan)
target_sources(SetteiMan
  PRIVATE
    settei_man/company.cpp
    settei_man/episode.cpp
    settei_man/materials.cpp
    settei_man/cuts.cpp
    settei_man/config.cpp
)
target_include_directories(SetteiMan PRIVATE settei_man ${Boost_INCLUDE_DIRS})
target_link_libraries(SetteiMan ${Boost_LIBRARIES} spdlog::spdlog APIs)

add_executable(Application)
target_sources(Application PRIVATE qt/main.cpp qt/main_window.cpp)
target_include_directories(Application PRIVATE settei_man ai qt)
target_link_libraries(Application SetteiMan APIs Qt6::Widgets Qt6::Core Qt6::Gui)
