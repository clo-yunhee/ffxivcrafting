include(CheckIncludeFile)

add_subdirectory(csprng)
add_subdirectory(imgui-cmake EXCLUDE_FROM_ALL)

add_library(openGA INTERFACE)
target_include_directories(openGA INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/openGA/src)