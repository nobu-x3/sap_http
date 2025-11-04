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
option(SAP_HTTP_USE_MODULES "Use C++20 modules (requires compiler support)" ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

set(CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/result.h
)

set(HTTP_HEADERS
)

set(HTTP_SOURCES
)

set(CORE_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/types.cppm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/result.cppm
)

set(HTTP_MODULES
)

if(SAP_HTTP_USE_MODULES)
    # Core module library
    add_library(sap_core)
    target_sources(sap_core
        PUBLIC
            FILE_SET CXX_MODULES FILES ${CORE_MODULES}
    )
    target_compile_features(sap_core PUBLIC cxx_std_20)
    
    # Shared library with modules
    if(SAP_HTTP_BUILD_SHARED)
        add_library(sap_http_shared SHARED)
        target_sources(sap_http_shared
            PUBLIC
                FILE_SET CXX_MODULES FILES ${HTTP_MODULES}
            PRIVATE
                ${HTTP_SOURCES}
        )
        target_link_libraries(sap_http_shared PUBLIC sap_core)
        target_compile_features(sap_http_shared PUBLIC cxx_std_20)
        
        if(WIN32)
            target_link_libraries(sap_http_shared PRIVATE ws2_32)
        endif()
        
        set_target_properties(sap_http_shared PROPERTIES
            OUTPUT_NAME sap_http
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    endif()
    
    # Static library with modules
    if(SAP_HTTP_BUILD_STATIC)
        add_library(sap_http_static STATIC)
        target_sources(sap_http_static
            PUBLIC
                FILE_SET CXX_MODULES FILES ${HTTP_MODULES}
            PRIVATE
                ${HTTP_SOURCES}
        )
        target_link_libraries(sap_http_static PUBLIC sap_core)
        target_compile_features(sap_http_static PUBLIC cxx_std_20)
        
        if(WIN32)
            target_link_libraries(sap_http_static PRIVATE ws2_32)
        endif()
        
        set_target_properties(sap_http_static PROPERTIES
            OUTPUT_NAME sap_http
            ARCHIVE_OUTPUT_NAME sap_http_static
        )
    endif()
else()
    # Traditional header-based build
    
    # Shared library without modules
    if(SAP_HTTP_BUILD_SHARED)
        add_library(sap_http_shared SHARED ${HTTP_SOURCES})
        target_include_directories(sap_http_shared
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
        )
        target_compile_features(sap_http_shared PUBLIC cxx_std_20)
        
        if(WIN32)
            target_link_libraries(sap_http_shared PRIVATE ws2_32)
        endif()
        
        set_target_properties(sap_http_shared PROPERTIES
            OUTPUT_NAME sap_http
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
    endif()
    
    # Static library without modules
    if(SAP_HTTP_BUILD_STATIC)
        add_library(sap_http_static STATIC ${HTTP_SOURCES})
        target_include_directories(sap_http_static
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
        )
        target_compile_features(sap_http_static PUBLIC cxx_std_20)
        
        if(WIN32)
            target_link_libraries(sap_http_static PRIVATE ws2_32)
        endif()
        
        set_target_properties(sap_http_static PROPERTIES
            OUTPUT_NAME sap_http
            ARCHIVE_OUTPUT_NAME sap_http_static
        )
    endif()
endif()

if(SAP_HTTP_BUILD_SHARED)
    add_library(sap::http ALIAS sap_http_shared)
elseif(SAP_HTTP_BUILD_STATIC)
    add_library(sap::http ALIAS sap_http_static)
endif()