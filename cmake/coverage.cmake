cmake_minimum_required(VERSION 3.7...3.30)

set(_var "${PROJECT_NAME}_USE_COVERAGE")
option(${_var} "Enable test coverage for ${PROJECT_NAME}" OFF)
message(STATUS "${_var} is ${${_var}}")
if(${_var})
  set(LOCAL_LIBRARY_TARGETS "")
  get_property(
    LOCAL_TARGETS
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY BUILDSYSTEM_TARGETS)

  # Iterate over all targets and filter out only library targets
  foreach(target ${LOCAL_TARGETS})
    get_target_property(TYPE ${target} TYPE)
    message(STATUS "Setting up '${target}' for coverage")
    if(TYPE STREQUAL "STATIC_LIBRARY"
       OR TYPE STREQUAL "SHARED_LIBRARY"
       OR TYPE STREQUAL "MODULE_LIBRARY")
      target_compile_options(
        ${target} PUBLIC -Og -g --coverage -fkeep-inline-functions
                         -fkeep-static-functions)
      target_link_options(${target} PUBLIC --coverage)
    endif()
  endforeach()
endif()
