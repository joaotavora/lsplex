cmake_minimum_required(VERSION 3.7...3.30)
option(USE_COVERAGE "Enable test coverage" OFF)
message("USE_COVERAGE is '${USE_COVERAGE}'")
if(USE_COVERAGE)
  # target_compile_options(Greeter PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  # target_link_options(Greeter PUBLIC -fprofile-arcs -ftest-coverage)

  set(LOCAL_LIBRARY_TARGETS "")
  get_property(LOCAL_TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY
    BUILDSYSTEM_TARGETS)


  # Iterate over all targets and filter out only library targets
  foreach(target ${LOCAL_TARGETS})
    get_target_property(TYPE ${target} TYPE)
    message(STATUS "Setting up '${target}' for coverage")
    if(TYPE STREQUAL "STATIC_LIBRARY" OR TYPE STREQUAL "SHARED_LIBRARY"
        OR TYPE STREQUAL "MODULE_LIBRARY")
      target_compile_options(${target} PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
      target_link_options(${target} PUBLIC -fprofile-arcs -ftest-coverage)
    endif()
  endforeach()
  
endif()
