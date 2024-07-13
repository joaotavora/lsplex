cmake_minimum_required(VERSION 3.4...3.30)

set(_var "${PROJECT_NAME}_USE_CCACHE")
option(${_var} "Use ccache when buildind ${PROJECT_NAME}" OFF)
set(CCACHE_OPTIONS
    ""
    CACHE STRING "options for ccache")
message(STATUS "${_var} is ${${_var}}")
if(${_var})
  find_program(CCACHE_PROGRAM ccache)

  if(CCACHE_PROGRAM)
    # Set up wrapper scripts
    set(C_LAUNCHER "${CCACHE_PROGRAM}")
    set(CXX_LAUNCHER "${CCACHE_PROGRAM}")
    set(CCCACHE_EXPORTS "")

    foreach(option ${CCACHE_OPTIONS})
      set(CCCACHE_EXPORTS "${CCCACHE_EXPORTS}\nexport ${option}")
    endforeach()

    configure_file(${CMAKE_CURRENT_LIST_DIR}/launch-c.in ccache.cmake/launch-c)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/launch-cxx.in
                   ccache.cmake/launch-cxx)

    set(CCACHE_LAUNCH_C "${CMAKE_CURRENT_BINARY_DIR}/ccache.cmake/launch-c")
    set(CCACHE_LAUNCH_CXX "${CMAKE_CURRENT_BINARY_DIR}/ccache.cmake/launch-cxx")

    execute_process(COMMAND chmod a+rx "${CCACHE_LAUNCH_C}"
                            "${CCACHE_LAUNCH_CXX}")

    if(CMAKE_GENERATOR STREQUAL "Xcode")
      # Set Xcode project attributes to route compilation and linking through
      # our scripts
      set(CMAKE_XCODE_ATTRIBUTE_CC
          "${CCACHE_LAUNCH_C}"
          CACHE INTERNAL "")
      set(CMAKE_XCODE_ATTRIBUTE_CXX
          "${CCACHE_LAUNCH_CXX}"
          CACHE INTERNAL "")
      set(CMAKE_XCODE_ATTRIBUTE_LD
          "${CCACHE_LAUNCH_C}"
          CACHE INTERNAL "")
      set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS
          "${CCACHE_LAUNCH_CXX}"
          CACHE INTERNAL "")
    else()
      # Support Unix Makefiles and Ninja
      set(CMAKE_C_COMPILER_LAUNCHER
          "${CCACHE_LAUNCH_C}"
          CACHE INTERNAL "")
      set(CMAKE_CXX_COMPILER_LAUNCHER
          "${CCACHE_LAUNCH_CXX}"
          CACHE INTERNAL "")
    endif()

    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
  else()
    message(WARNING "Could not enable Ccache: could not find program")
  endif()

endif()
