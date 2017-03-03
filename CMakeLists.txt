cmake_minimum_required (VERSION 3.1)
project (grunge)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor/libtwittercpp)
add_subdirectory(vendor/verbly)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)

find_package(PkgConfig)
pkg_check_modules(GraphicsMagick GraphicsMagick++ REQUIRED)

include_directories(${GraphicsMagick_INCLUDE_DIRS} vendor/verbly/lib vendor/libtwittercpp/src vendor/yaml-cpp/include vendor/libtwittercpp/vendor/curlcpp/include)
link_directories(${GraphicsMagick_LIBRARY_DIRS})
add_executable(grunge main.cpp grunge.cpp palette.cpp)
set_property(TARGET grunge PROPERTY CXX_STANDARD 11)
set_property(TARGET grunge PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(grunge ${GraphicsMagick_LIBRARIES} verbly yaml-cpp twitter++)

