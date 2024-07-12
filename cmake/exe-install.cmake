function(setup_exe_install package)
  cmake_parse_arguments(EXE_INSTALL "" "" "TARGETS" ${ARGN} )
  install(
    TARGETS ${EXE_INSTALL_TARGETS}
    RUNTIME COMPONENT ${package}_Runtime
  )
endfunction()
