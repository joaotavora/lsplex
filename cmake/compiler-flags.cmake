get_property(
  LOCAL_TARGETS
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY BUILDSYSTEM_TARGETS)

foreach(target ${LOCAL_TARGETS})
  message(STATUS "Setting up '${target}' compiler quirks")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
                                              "GNU")
    target_compile_options(
      ${target}
      PUBLIC -Wall
             -Wpedantic
             -Wextra
             -Werror
             -fstack-protector-strong
             -Wsign-conversion
             -Wconversion
             -Wunused-result
             -Wformat=2
             -Wundef
             -Werror=float-equal
             -Wshadow
             -Wcast-align
             -Wnull-dereference
             -Wdouble-promotion
             -Wimplicit-fallthrough
             -Wextra-semi
             -Woverloaded-virtual
             -Wnon-virtual-dtor
             -Wold-style-cast)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND MINGW)
       target_link_libraries(${target} PRIVATE bcrypt wsock32) # reasons
     endif()
  elseif(MSVC)
    target_compile_options(${target} PUBLIC /W4 /WX)
    target_compile_definitions(${target} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endforeach()
