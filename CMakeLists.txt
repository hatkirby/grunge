cmake_minimum_required (VERSION 3.1)
project (grunge)

add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/verbly)

find_package(PkgConfig)
pkg_check_modules(yaml-cpp yaml-cpp REQUIRED)
pkg_check_modules(GraphicsMagick GraphicsMagick++ REQUIRED)

include_directories(
  vendor/libtwittercpp/src
  vendor/libtwittercpp/vendor/curlcpp/include
  vendor/verbly/lib
  ${yaml-cpp_INCLUDE_DIRS}
  ${GraphicsMagick_INCLUDE_DIRS})

link_directories(${GraphicsMagick_LIBRARY_DIRS})

add_executable(grunge main.cpp grunge.cpp palette.cpp)
set_property(TARGET grunge PROPERTY CXX_STANDARD 11)
set_property(TARGET grunge PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(grunge ${yaml-cpp_LIBRARIES} ${GraphicsMagick_LIBRARIES} verbly twitter++)
