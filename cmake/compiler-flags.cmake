get_property(
  LOCAL_TARGETS
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY BUILDSYSTEM_TARGETS)

foreach(target ${LOCAL_TARGETS})
  message(STATUS "Setting up '${target}' compiler quirks")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
                                              "GNU")
    target_compile_options(${target} PUBLIC -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(${target} PUBLIC /W4 /WX)
    target_compile_definitions(${target} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endforeach()
