cmake_minimum_required(VERSION 3.15)
project(sap_http VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

option(SAP_HTTP_BUILD_SHARED "Build shared library" ON)
option(SAP_HTTP_BUILD_STATIC "Build static library" ON)
option(SAP_HTTP_BUILD_TESTS "Build tests" ON)
option(SAP_HTTP_INSTALL "Install library" ON)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

set(HTTP_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/net/http.h
)

set(HTTP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/headers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/url.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/request.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/response.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/net/server.cpp
)

# Shared library
if(SAP_HTTP_BUILD_SHARED)
    add_library(sap_http_shared SHARED ${HTTP_SOURCES})
    target_include_directories(sap_http_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sap_core/include>
            $<INSTALL_INTERFACE:sap_core/include>
            $<INSTALL_INTERFACE:include>
    )

    if(WIN32)
        target_link_libraries(sap_http_shared PRIVATE ws2_32)
    endif()

    set_target_properties(sap_http_shared PROPERTIES
        OUTPUT_NAME sap_http
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Static library
if(SAP_HTTP_BUILD_STATIC)
    add_library(sap_http_static STATIC ${HTTP_SOURCES})
    target_include_directories(sap_http_static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sap_core/include>
            $<INSTALL_INTERFACE:sap_core/include>
            $<INSTALL_INTERFACE:include>
    )

    if(WIN32)
        target_link_libraries(sap_http_static PRIVATE ws2_32)
    endif()

    set_target_properties(sap_http_static PROPERTIES
        OUTPUT_NAME sap_http
        ARCHIVE_OUTPUT_NAME sap_http_static
    )
endif()

# Create alias
if(SAP_HTTP_BUILD_SHARED)
    add_library(sap::http ALIAS sap_http_shared)
elseif(SAP_HTTP_BUILD_STATIC)
    add_library(sap::http ALIAS sap_http_static)
endif()

# Tests
if(SAP_HTTP_BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(sap_http_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/url_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/headers_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/request_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/response_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/client_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/server_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_tests.cpp
    )

    if(SAP_HTTP_BUILD_STATIC)
        target_link_libraries(sap_http_tests PRIVATE sap_http_static)
    else()
        target_link_libraries(sap_http_tests PRIVATE sap_http_shared)
    endif()

    target_link_libraries(sap_http_tests PRIVATE
        GTest::gtest_main
        GTest::gmock
    )

    include(GoogleTest)
    gtest_discover_tests(sap_http_tests)
endif()

# Installation
if(SAP_HTTP_INSTALL)
    include(GNUInstallDirs)

    # Install targets
    if(SAP_HTTP_BUILD_SHARED)
        install(TARGETS sap_http_shared
            EXPORT sap_http_targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    if(SAP_HTTP_BUILD_STATIC)
        install(TARGETS sap_http_static
            EXPORT sap_http_targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    # Install headers
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install export (only once, regardless of shared/static)
    install(EXPORT sap_http_targets
        FILE sap_http_targets.cmake
        NAMESPACE sap::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sap_http
    )

    # Create and install config file
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/sap_http_config_version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
    )

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sap_http_config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/sap_http_config.cmake"
        @ONLY
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/sap_http_config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/sap_http_config_version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sap_http
    )
endif()

# Configuration summary
message(STATUS "")
message(STATUS "sap_http Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build shared library: ${SAP_HTTP_BUILD_SHARED}")
message(STATUS "  Build static library: ${SAP_HTTP_BUILD_STATIC}")
message(STATUS "  Build tests: ${SAP_HTTP_BUILD_TESTS}")
message(STATUS "  Install: ${SAP_HTTP_INSTALL}")
message(STATUS "")