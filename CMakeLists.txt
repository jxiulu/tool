cmake_minimum_required(VERSION 3.28)
project(sakugatranslator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD ON)
set(EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/lib/jwt/include)

set(SRCFILES src/main.cpp src/framework.hpp)
add_executable(main ${SRCFILES})

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/src
                                        ${CMAKE_SOURCE_DIR}/lib/jwt/include)
target_link_libraries(main PRIVATE CURL::libcurl OpenSSL::SSL OpenSSL::Crypto
                                   nlohmann_json::nlohmann_json)

if(DEFINED OPENSSL_INCLUDE_DIR)
  target_include_directories(main PRIVATE "${OPENSSL_INCLUDE_DIR}")
elseif(DEFINED OPENSSL_INCLUDE_DIRS)
  target_include_directories(main PRIVATE ${OPENSSL_INCLUDE_DIRS})
endif()
