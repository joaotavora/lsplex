cmake_minimum_required(VERSION 3.14...3.30)
project(Greeter VERSION 1.0.0 LANGUAGES CXX)
include(cmake/prelude.cmake)
include(cmake/ccache.cmake)

# --- Library target ---
file(GLOB sources CONFIGURE_DEPENDS src/libgreeter/*.cpp)

add_library(Greeter_greeter ${sources})
add_library(greeter::greeter ALIAS Greeter_greeter)
configure_file(include/greeter/version.h.in include/greeter/version.h @ONLY)

set_target_properties(
  Greeter_greeter PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  EXPORT_NAME Greeter         # TODO: casing?
  OUTPUT_NAME Greeter
)
target_include_directories(
  Greeter_greeter ${warning_guard}
  PUBLIC
  "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
)
# TODO: missing include directories for export directory

target_link_libraries(Greeter_greeter PRIVATE fmt::fmt)

# --- Executable target ---
file(GLOB sources CONFIGURE_DEPENDS src/greeter/*.cpp)
add_executable(Greeter_exe ${sources})
add_executable(Greeter::exe ALIAS Greeter_exe)
set_property(TARGET Greeter_exe PROPERTY OUTPUT_NAME Greeter)

target_link_libraries(Greeter_exe Greeter_greeter cxxopts::cxxopts)

find_package(cxxopts REQUIRED)
find_package(fmt REQUIRED)

include(cmake/compiler-features.cmake)

if(NOT Greeter_DEV)
  # Installation code goes here, no?
  return()
endif()

# --- Dev helpers ---
include(cmake/sanitizers.cmake)
include(cmake/coverage.cmake)
include(cmake/compiler-flags.cmake)

# --- Testing ---
find_package(doctest REQUIRED)
include(doctest)

file(GLOB sources CONFIGURE_DEPENDS "./test/*.cpp")
add_executable(Greeter_tests ${sources})
target_link_libraries(Greeter_tests Greeter_greeter doctest::doctest)

doctest_discover_tests(Greeter_tests)
enable_testing()
add_test(NAME Greeter_tests COMMAND Greeter_tests)
