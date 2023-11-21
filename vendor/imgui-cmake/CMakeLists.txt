set(imgui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../imgui)

add_library(ImGui_Core STATIC)
target_include_directories(ImGui_Core PUBLIC ${imgui_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(ImGui_Core PRIVATE
    ${imgui_SOURCE_DIR}/imconfig.h
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui_internal.h)
file(GLOB _imgui_sources ${imgui_SOURCE_DIR}/imgui*.cpp)
foreach(SOURCE ${_imgui_sources})
    target_sources(ImGui_Core 
            PRIVATE ${SOURCE})
endforeach()
target_compile_definitions(ImGui_Core PUBLIC "ImDrawIdx=unsigned int")
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(ImGui_Core PUBLIC imm32 dwmapi)
endif()
if(APPLE)
    target_compile_definitions(ImGui_Core PUBLIC "IMGUI_ENABLE_OSX_DEFAULT_CLIPBOARD_FUNCTIONS")
    target_link_libraries(ImGui_Core PUBLIC "-framework ApplicationServices")
endif()
add_library(ImGui::Core ALIAS ImGui_Core)

function(imgui_library TARGET)
    cmake_parse_arguments(TARGET "" "" "HEADERS;SOURCES;DEFINITIONS;TARGETS;INCLUDE_DIRECTORIES" ${ARGN})
    set(_target ImGui_${TARGET})
    add_library(${_target} STATIC)
    target_link_libraries(${_target} PUBLIC ImGui_Core)
    foreach(SOURCE ${TARGET_SOURCES})
        target_sources(${_target}
                PRIVATE ${imgui_SOURCE_DIR}/${SOURCE})
    endforeach()
    foreach(DEFINITION ${TARGET_DEFINITIONS})
        target_compile_definitions(${_target}
            PUBLIC ${DEFINITION})
    endforeach()
    target_include_directories(${_target}
            PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>)
    foreach(INCLUDE_DIRECTORY ${TARGET_INCLUDE_DIRECTORIES})
        target_include_directories(${_target}
                PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/${INCLUDE_DIRECTORY}>)
    endforeach()
    foreach(DEFINITION ${TARGET_DEFINITIONS})
        target_compile_definitions(${_target}
            PUBLIC ${DEFINITION})
    endforeach()
    foreach(INTERFACE_TARGET ${TARGET_TARGETS})
        target_link_libraries(${_target}
            PUBLIC ${INTERFACE_TARGET})
    endforeach()
    add_library(ImGui::${TARGET} ALIAS ${_target})
endfunction()

imgui_library(ImplWin32
    HEADERS backends/imgui_impl_win32.h
    SOURCES backends/imgui_impl_win32.cpp
    INCLUDE_DIRECTORIES backends)

if(NOT MSVC)
    target_link_libraries(ImGui_ImplWin32 PUBLIC dwmapi)
endif()

imgui_library(ImplGlfw
    HEADERS backends/imgui_impl_glfw.h
    SOURCES backends/imgui_impl_glfw.cpp
    INCLUDE_DIRECTORIES backends)

if(EMSCRIPTEN)
    target_link_options(ImGui_ImplGlfw PUBLIC "SHELL:-s USE_GLFW=3")
else()
    target_link_libraries(ImGui_ImplGlfw PUBLIC glfw) 
endif()

imgui_library(ImplOpenGL3
    HEADERS
        backends/imgui_impl_opengl3.h
        backends/imgui_impl_opengl3_loader.h
    SOURCES backends/imgui_impl_opengl3.cpp
    DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_GLAD=1"
    INCLUDE_DIRECTORIES backends)

find_package(OpenGL QUIET)
if(OpenGL_FOUND)
    target_link_libraries(ImGui_ImplOpenGL3 PUBLIC OpenGL::GL glad)
endif()

imgui_library(ImplVulkan
    HEADERS backends/imgui_impl_vulkan.h
    SOURCES backends/imgui_impl_vulkan.cpp
    INCLUDE_DIRECTORIES backends)

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_link_libraries(ImGui_ImplVulkan PUBLIC Vulkan::Vulkan)
endif()

imgui_library(ImplDx9
    HEADERS backends/imgui_impl_dx9.h
    SOURCES backends/imgui_impl_dx9.cpp
    INCLUDE_DIRECTORIES backends)

check_include_file(d3d9.h HAS_D3D9)
if(HAS_D3D9)
    target_link_libraries(ImGui_ImplDx9 PUBLIC d3d9)
    if(NOT MSVC)
        target_link_libraries(ImGui_ImplDx9 PUBLIC d3dcompiler)
    endif()
endif()

imgui_library(ImplDx11
    HEADERS backends/imgui_impl_dx11.h
    SOURCES backends/imgui_impl_dx11.cpp
    INCLUDE_DIRECTORIES backends)

check_include_file(d3d11.h HAS_D3D11)
if(HAS_D3D11)
    target_link_libraries(ImGui_ImplDx11 PUBLIC d3d11)
    if(NOT MSVC)
        target_link_libraries(ImGui_ImplDx11 PUBLIC d3dcompiler)
    endif()
endif()

imgui_library(FreeType
    HEADERS misc/freetype/imgui_freetype.h
    SOURCES misc/freetype/imgui_freetype.cpp
    DEFINITIONS
        "IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION"
        "IMGUI_ENABLE_FREETYPE"
    TARGETS freetype)

imgui_library(StdLib
    HEADERS misc/cpp/imgui_stdlib.h
    SOURCES misc/cpp/imgui_stdlib.cpp)