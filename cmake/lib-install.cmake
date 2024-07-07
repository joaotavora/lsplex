set(package_config_template ${CMAKE_CURRENT_LIST_DIR}/package-config.cmake.in)

if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/${PROJECT_NAME}-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

macro(setup_lib_install package)
  set(multiValueArgs TARGETS DEPS)
  cmake_parse_arguments(LIB_INSTALL "" "" "${multiValueArgs}" ${ARGN} )

  install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT ${package}_Development
    PATTERN "version.h.in" EXCLUDE
  )

  install(
    TARGETS ${LIB_INSTALL_TARGETS}
    EXPORT ${package}Targets
    RUNTIME #
    COMPONENT ${package}_Runtime
    LIBRARY #
    COMPONENT ${package}_Runtime
    NAMELINK_COMPONENT ${package}_Development
    ARCHIVE #
    COMPONENT ${package}_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
  )

  # Allow package maintainers to freely override the path for the configs
  set(
    ${package}_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
  )
  set_property(CACHE ${package}_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
  mark_as_advanced(${package}_INSTALL_CMAKEDIR)

  include(CMakePackageConfigHelpers)
  # generate the config file that includes the exports
  set(PACKAGE ${package})
  set(FIND_DEPENDENCIES "")
  foreach(dep ${LIB_INSTALL_DEPS})
    set(FIND_DEPENDENCIES "${FIND_DEPENDENCIES}find_dependency(${dep})\n")
  endforeach()
  configure_package_config_file(
    ${package_config_template}
    "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake"
    INSTALL_DESTINATION "${${package}_INSTALL_CMAKEDIR}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )

  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake"
    DESTINATION "${${package}_INSTALL_CMAKEDIR}"
    COMPONENT ${package}_Development
  )

  install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${${package}_INSTALL_CMAKEDIR}"
    COMPONENT ${package}_Development
  )

  install(
    EXPORT ${package}Targets
    NAMESPACE ${package}::
    DESTINATION "${${package}_INSTALL_CMAKEDIR}"
    COMPONENT ${package}_Development
  )
endmacro()
