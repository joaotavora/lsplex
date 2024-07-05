cmake_minimum_required(VERSION 3.15...3.30)
project(Greeter VERSION 1.0.0 LANGUAGES CXX)

find_package(ZLIB REQUIRED)

file(GLOB sources CONFIGURE_DEPENDS src/greeter/*.cpp)
add_executable(Greeter_exe ${sources})
target_link_libraries(Greeter_exe ZLIB::ZLIB)
