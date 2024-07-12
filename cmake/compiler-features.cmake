get_property(
  LOCAL_TARGETS
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  PROPERTY BUILDSYSTEM_TARGETS)
foreach(target ${LOCAL_TARGETS})
  message(STATUS "Setting up '${target}' compile features to C++20")
  target_compile_features("${target}" PRIVATE cxx_std_20)
endforeach()
