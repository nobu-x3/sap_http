cmake_minimum_required(VERSION 3.28)
project(sap_http VERSION 1.0.0 LANGUAGES CXX)

# C++20 required for modules
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

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

if(SAP_HTTP_INSTALL)
    include(GNUInstallDirs)
    
    if(SAP_HTTP_USE_MODULES)
        # Install module-based targets
        if(SAP_HTTP_BUILD_SHARED)
            install(TARGETS sap_http_shared sap_core
                EXPORT sap_http_shared_targets
                FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
        
        if(SAP_HTTP_BUILD_STATIC)
            install(TARGETS sap_http_static sap_core
                EXPORT sap_http_static_targets
                FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    else()
        # Install header-based targets
        if(SAP_HTTP_BUILD_SHARED)
            install(TARGETS sap_http_shared
                EXPORT sap_http_shared_targets
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
        
        if(SAP_HTTP_BUILD_STATIC)
            install(TARGETS sap_http_static
                EXPORT sap_http_static_targets
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
        
        # Install headers
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    endif()
    
    # Install export
    if(SAP_HTTP_BUILD_SHARED)
        install(EXPORT sap_http_static_targets
            FILE sap_http_targets.cmake
            NAMESPACE sap::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sap_http
        )
    endif()
    if(SAP_HTTP_BUILD_STATIC)
        install(EXPORT sap_http_shared_targets
        FILE sap_http_targets.cmake
        NAMESPACE sap::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sap_http
    )
    endif()
    
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

message(STATUS "")
message(STATUS "sap_http Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Use C++20 Modules: ${SAP_HTTP_USE_MODULES}")
message(STATUS "  Build shared library: ${SAP_HTTP_BUILD_SHARED}")
message(STATUS "  Build static library: ${SAP_HTTP_BUILD_STATIC}")
message(STATUS "  Build tests: ${SAP_HTTP_BUILD_TESTS}")
message(STATUS "  Install: ${SAP_HTTP_INSTALL}")
message(STATUS "")