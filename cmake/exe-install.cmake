macro(setup_exe_install)
  set(multiValueArgs TARGETS)
  cmake_parse_arguments(EXE_INSTALL "${}" "${}" "${multiValueArgs}" ${ARGN} )
  install(
    TARGETS ${EXE_INSTALL_TARGETS}
    RUNTIME COMPONENT ${package}_Runtime
  )
endmacro()
