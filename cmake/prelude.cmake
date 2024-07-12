# Most of this stuff lifted from https://github.com/friendlyanon/cmake-init
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not supported. ")
endif()

# This variable is set by project() in CMake 3.21+, but we opt not to require
# that
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}"
               PROJECT_IS_TOP_LEVEL)

# ---- Developer mode ----
# Developer mode enables targets and code paths in the CMake scripts that are
# only relevant for the developer(s) of ${PROJECT_NAME}. Targets necessary to
# build the project must be provided unconditionally, so consumers can trivially
# build and package the project
if(PROJECT_IS_TOP_LEVEL)
  option(${PROJECT_NAME}_DEV "Enable developer mode" OFF)
endif()

# ---- Warning guard ----

# target_include_directories with the SYSTEM modifier will request the compiler
# to omit warnings from the provided paths, if the compiler supports that This
# is to provide a user experience similar to find_package when add_subdirectory
# or FetchContent is used to consume this project
set(warning_guard "")
if(NOT PROJECT_IS_TOP_LEVEL)
  option(
    ${PROJECT_NAME}_INCLUDES_WITH_SYSTEM
    "Use SYSTEM modifier for ${PROJECT_NAME}'s includes, disabling warnings" ON)
  mark_as_advanced(${PROJECT_NAME}_INCLUDES_WITH_SYSTEM)
  if(${PROJECT_NAME}_INCLUDES_WITH_SYSTEM)
    set(warning_guard SYSTEM)
  endif()
endif()

if(${PROJECT_NAME}_DEV)
  message(STATUS "THIS IS DEVELOPER MODE YAY")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()
