# More or less lifted from https://github.com/StableCoder/cmake-scripts


function(append value)
  foreach(variable ${ARGN})
    set(${variable}
        "${${variable}} ${value}"
        PARENT_SCOPE)
  endforeach(variable)
endfunction()


set(_var "${PROJECT_NAME}_USE_SANITIZER")
option(${_var}
    "Compile with a sanitizer. Options are: Address, Memory, MemoryWithOrigins, Undefined, Thread, Leak, 'Address;Undefined'"
    "")
message(STATUS "${_var} is ${${_var}}")
if(${_var})
  append("-fno-omit-frame-pointer" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)

  if(UNIX)

    if(uppercase_CMAKE_BUILD_TYPE STREQUAL "DEBUG")
      append("-O1" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    endif()

    if(${_var} MATCHES "([Aa]ddress);([Uu]ndefined)"
       OR ${_var} MATCHES "([Uu]ndefined);([Aa]ddress)")
      message(STATUS "Building with Address, Undefined sanitizers")
      append("-fsanitize=address,undefined" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    elseif(${_var} MATCHES "([Aa]ddress)")
      # Optional: -fno-optimize-sibling-calls -fsanitize-address-use-after-scope
      message(STATUS "Building with Address sanitizer")
      append("-fsanitize=address" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    elseif(${_var} MATCHES "([Mm]emory([Ww]ith[Oo]rigins)?)")
      # Optional: -fno-optimize-sibling-calls -fsanitize-memory-track-origins=2
      append("-fsanitize=memory" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
      if(${_var} MATCHES "([Mm]emory[Ww]ith[Oo]rigins)")
        message(STATUS "Building with MemoryWithOrigins sanitizer")
        append("-fsanitize-memory-track-origins" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
      else()
        message(STATUS "Building with Memory sanitizer")
      endif()
    elseif(${_var} MATCHES "([Uu]ndefined)")
      message(STATUS "Building with Undefined sanitizer")
      append("-fsanitize=undefined" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
      if(EXISTS "${BLACKLIST_FILE}")
        append("-fsanitize-blacklist=${BLACKLIST_FILE}" CMAKE_C_FLAGS
               CMAKE_CXX_FLAGS)
      endif()
    elseif(${_var} MATCHES "([Tt]hread)")
      message(STATUS "Building with Thread sanitizer")
      append("-fsanitize=thread" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    elseif(${_var} MATCHES "([Ll]eak)")
      message(STATUS "Building with Leak sanitizer")
      append("-fsanitize=leak" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    else()
      message(
        FATAL_ERROR "Unsupported value of ${_var}: ${${_var}}")
    endif()
  elseif(MSVC)
    if(${_var} MATCHES "([Aa]ddress)")
      message(STATUS "Building with Address sanitizer")
      append("-fsanitize=address" CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    else()
      message(
        FATAL_ERROR
          "This sanitizer not yet supported in the MSVC environment: ${${_var}}"
      )
    endif()
  else()
    message(FATAL_ERROR "${_var} is not supported on this platform.")
  endif()

endif()
