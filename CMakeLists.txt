cmake_minimum_required(VERSION 3.28)
project(sap_http VERSION 1.0.0 LANGUAGES CXX)

# C++20 required for modules
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAP_HTTP_BUILD_SHARED "Build shared library" ON)
option(SAP_HTTP_BUILD_STATIC "Build static library" ON)
option(SAP_HTTP_BUILD_TESTS "Build tests" ON)
option(SAP_HTTP_INSTALL "Install library" ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

add_library(sap_core)
target_sources(sap_core
    PUBLIC
        FILE_SET CXX_MODULES FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/types.cppm
            ${CMAKE_CURRENT_SOURCE_DIR}/src/core/result.cppm
)
target_compile_features(sap_core PUBLIC cxx_std_20)