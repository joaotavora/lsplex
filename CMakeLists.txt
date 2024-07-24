cmake_minimum_required(VERSION 3.14...3.30)
project(
  LsPlex
  VERSION 1.0.0
  LANGUAGES CXX)
include(cmake/prelude.cmake)

if(LsPlex_DOCS)
  add_subdirectory("./doc")
  if(LsPlex_DOCS STREQUAL "SKIP_OTHER_TARGETS")
    return()
  endif()
endif()

include(cmake/ccache.cmake)

# --- A library target
#
# There could be more such sections (so _lib1, _lib2 instead of _lib). Note
# OUTPUT_NAME is 'lsplex' so this will likely become liblsplex.a/so on linux.
file(GLOB sources CONFIGURE_DEPENDS src/liblsplex/*.cpp)
add_library(${PROJECT_NAME}_lib ${sources})
add_library(lsplex::lsplex ALIAS ${PROJECT_NAME}_lib)
configure_file(include/lsplex/version.h.in include/lsplex/version.h @ONLY)

set_target_properties(
  ${PROJECT_NAME}_lib
  PROPERTIES CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES
             VERSION "${PROJECT_VERSION}"
             SOVERSION "${PROJECT_VERSION_MAJOR}"
             EXPORT_NAME ${PROJECT_NAME} # Tweak if more than one lib.
             OUTPUT_NAME lsplex)
target_include_directories(
  ${PROJECT_NAME}_lib ${warning_guard}
  PUBLIC "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
         "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>")
include(GenerateExportHeader)
generate_export_header(
  ${PROJECT_NAME}_lib
  BASE_NAME
  lsplex
  EXPORT_FILE_NAME
  include/lsplex/export.hpp
  CUSTOM_CONTENT_FROM_VARIABLE
  pragma_suppress_c4251)

target_link_libraries(
  ${PROJECT_NAME}_lib
  PUBLIC Boost::json Boost::boost
  PRIVATE fmt::fmt)

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/lib-install.cmake)
  setup_lib_install(${PROJECT_NAME} TARGETS ${PROJECT_NAME}_lib DEPS fmt Boost)
endif()

# --- An executable target.
#
# Note OUTPUT_NAME also lowercase 'lsplex'
file(GLOB sources CONFIGURE_DEPENDS src/lsplex/*.cpp)
add_executable(${PROJECT_NAME}_exe ${sources})
add_executable(${PROJECT_NAME}::exe ALIAS ${PROJECT_NAME}_exe)
set_property(TARGET ${PROJECT_NAME}_exe PROPERTY OUTPUT_NAME lsplex)

target_link_libraries(${PROJECT_NAME}_exe PRIVATE ${PROJECT_NAME}_lib fmt::fmt
                                                  cxxopts::cxxopts)

find_package(cxxopts REQUIRED)
find_package(fmt REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/exe-install.cmake)
  setup_exe_install(${PROJECT_NAME} TARGETS ${PROJECT_NAME}_exe)
endif()

# --- Dev cutoff ---
if(NOT ${PROJECT_NAME}_DEV)
  return()
endif()

# --- Testing ---
find_package(doctest REQUIRED)
include(doctest)

file(GLOB sources CONFIGURE_DEPENDS "./test/*.cpp")
add_executable(${PROJECT_NAME}_tests ${sources})
target_link_libraries(${PROJECT_NAME}_tests ${PROJECT_NAME}_lib
                      Boost::json Boost::boost
                      doctest::doctest)

file(COPY test/resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

doctest_discover_tests(${PROJECT_NAME}_tests)
enable_testing()

# --- Dev stuff ---
include(CPack)
include(cmake/sanitizers.cmake)
include(cmake/coverage.cmake)
include(cmake/compiler-features.cmake)
include(cmake/compiler-flags.cmake)

