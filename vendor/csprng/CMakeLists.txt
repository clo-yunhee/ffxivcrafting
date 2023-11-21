project(CSPRNG)

add_library(csprng STATIC csprng.c)
target_include_directories(csprng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)