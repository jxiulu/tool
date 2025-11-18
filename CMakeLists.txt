cmake_minimum_required(VERSION 4.1.0)
project(settei_man LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQURIED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)

add_executable(SetteiMan
    settei_man/settei_man.cpp
    settei_man/company.cpp
    settei_man/episode.cpp
    settei_man/mats.cpp
)
target_include_directories(SetteiMan PRIVATE settei_man ${Boost_INCLUDE_DIRS})
target_link_libraries(SetteiMan ${Boost_LIBRARIES} spdlog::spdlog)
