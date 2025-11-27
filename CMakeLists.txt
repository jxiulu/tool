cmake_minimum_required(VERSION 4.1.0)

project(settei_man LANGUAGES CXX)

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

add_library(apis)
target_sources(apis PRIVATE 
    apis/ai.cpp
    apis/deepl.cpp
)
target_include_directories(apis PUBLIC apis/ settei_man/)
target_link_libraries(apis PUBLIC nlohmann_json::nlohmann_json CURL::libcurl)

add_library(SetteiMan)
target_sources(SetteiMan
  PRIVATE
    settei_man/settei_man.cpp
    settei_man/company.cpp
    settei_man/episode.cpp
    settei_man/materials.cpp
    settei_man/cuts.cpp
)
target_include_directories(SetteiMan PRIVATE settei_man ${Boost_INCLUDE_DIRS})
target_link_libraries(SetteiMan ${Boost_LIBRARIES} spdlog::spdlog apis)

add_executable(Application)
target_sources(Application PRIVATE qt/main.cpp qt/mainwin.cpp)
target_include_directories(Application PRIVATE settei_man ai qt)
target_link_libraries(Application SetteiMan apis Qt6::Widgets Qt6::Core Qt6::Gui)
