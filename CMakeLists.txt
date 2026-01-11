cmake_minimum_required(VERSION 3.22)

project(wrap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" ON)

find_package(fmt CONFIG REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(proxygen CONFIG REQUIRED)

add_library(wrap)

target_include_directories(wrap
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(wrap
  PRIVATE
    src/app.cpp
    src/wrap.cpp
)

target_compile_definitions(wrap PUBLIC
  WRAP_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(wrap
  PUBLIC
    fmt::fmt
    Folly::folly
  PRIVATE
    proxygen::proxygenhttpserver
)

set_target_properties(wrap PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

add_library(wrap::wrap ALIAS wrap)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples/hello)
  add_subdirectory(examples/middleware)
endif()

include(GNUInstallDirs)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS wrap
  EXPORT wrap-targets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT wrap-targets
  FILE wrap-targets.cmake
  NAMESPACE wrap::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wrap
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/wrap-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/wrap-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wrap
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/wrap-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/wrap-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/wrap-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wrap
)
