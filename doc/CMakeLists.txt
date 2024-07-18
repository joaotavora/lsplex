include(FetchContent)

FetchContent_Declare(
  mcss
  GIT_REPOSITORY https://github.com/mosra/m.css.git
  GIT_TAG        master
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/mcss"
  UPDATE_DISCONNECTED YES
    ${extract_timestamps}
)
FetchContent_MakeAvailable(mcss)

find_package(Python3 3.6 REQUIRED)

set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
configure_file(./Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(./conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py)

add_custom_target(
  LsPlexDocs
  ${CMAKE_COMMAND} -E make_directory "${DOXYGEN_OUTPUT_DIRECTORY}"
  COMMAND "${CMAKE_CURRENT_BINARY_DIR}/mcss/documentation/doxygen.py"
  "${CMAKE_CURRENT_BINARY_DIR}/conf.py"
  COMMAND echo "Docs written to: ${DOXYGEN_OUTPUT_DIRECTORY}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
