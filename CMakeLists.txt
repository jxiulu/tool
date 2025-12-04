cmake_minimum_required(VERSION 4.1.0)

project(Setman LANGUAGES CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES ENABLE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_library(SetmanAIEndpoints)
target_sources(
  SetmanAIEndpoints
  PRIVATE setman/ai_endpoints/curl_helpers.cpp setman/ai_endpoints/deepl.cpp
          setman/ai_endpoints/openrouter.cpp setman/ai_endpoints/google.cpp)
target_include_directories(SetmanAIEndpoints PUBLIC setman/ai-endpoints/
                                                    setman/)
target_link_libraries(SetmanAIEndpoints nlohmann_json::nlohmann_json
                      CURL::libcurl)

add_library(SetmanMaterials)
target_sources(
  SetmanMaterials
  PRIVATE setman/materials/material.cpp setman/materials/cut.cpp
          setman/materials/image.cpp setman/materials/element.cpp)
target_include_directories(SetmanMaterials PUBLIC setman/materials setman/)
target_link_libraries(SetmanMaterials spdlog::spdlog
                      SetmanCore)

add_library(SetmanCore)
target_sources(
  SetmanCore PRIVATE setman/episode.cpp setman/series.cpp setman/error.cpp
                     setman/company.cpp setman/config.cpp)
target_include_directories(SetmanCore PUBLIC setman ${Boost_INCLUDE_DIRS})
target_link_libraries(SetmanCore SetmanMaterials SetmanAIEndpoints ${Boost_LIBRARIES})

add_library(SetmanConversationsModule)
target_sources(SetmanConversationsModule PRIVATE
setman/conversations/conversations.cpp)
target_include_directories(SetmanConversationsModule PUBLIC setman/)

add_library(SetmanTranslationService)
target_sources(
  SetmanTranslationService
  PRIVATE setman/translation_service/translation_service.cpp
          setman/translation_service/response_converters.cpp)
target_include_directories(
  SetmanTranslationService PUBLIC setman/ setman/ai_endpoints
                                  setman/translation_service)
target_link_libraries(SetmanTranslationService SetmanAIEndpoints
SetmanConversationsModule CURL::libcurl)

add_executable(Application)
target_sources(Application PRIVATE qt/main.cpp qt/main_window.cpp)
target_include_directories(Application PRIVATE qt/ setman/)
target_link_libraries(
  Application
  SetmanCore
  SetmanMaterials
  SetmanTranslationService
  SetmanAIEndpoints
  Qt6::Widgets
  Qt6::Core
  Qt6::Gui)
