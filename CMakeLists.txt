cmake_minimum_required(VERSION 3.10)
project(vk_engine)

# download ffmpeg bin from https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z there and paste it in the external/ffmpeg folder
# structure of the ffmpeg folder should be like this:
#├───bin
#├───doc
#├───include
#│   ├───libavcodec
#│   ├───libavdevice
#│   ├───libavfilter
#│   ├───libavformat
#│   ├───libavutil
#│   ├───libswresample
#│   └───libswscale
#├───lib
#└───presets


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DVK_NO_PROTOTYPES)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/*.cc
    external/utils/volk.c
)

file(GLOB_RECURSE INCLUDE_FILES
    src/include/*.hh
)

add_subdirectory(external/glfw)
add_subdirectory(external/imgui)
find_package(Vulkan REQUIRED)

add_executable(vk_engine
   ${SRC_FILES}
   ${INCLUDE_FILES}
        src/include/demo.hh
        src/demo.cc
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_vulkan.cpp
        "src/include/miniaudio.h")

set_target_properties(${project_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/assets")

target_include_directories(vk_engine PRIVATE src/include/ external/glm/ external/utils/ external/ffmpeg/include/  external/imgui/)

target_link_libraries(vk_engine
    glfw
    Vulkan::Vulkan
    ImGui
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avcodec.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avformat.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avutil.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/swscale.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/swresample.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avfilter.lib
)


add_custom_command(TARGET vk_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/ffmpeg/bin/avcodec-62.dll"
        "${CMAKE_SOURCE_DIR}/external/ffmpeg/bin/avformat-62.dll"
        "${CMAKE_SOURCE_DIR}/external/ffmpeg/bin/avutil-60.dll"
        "${CMAKE_SOURCE_DIR}/external/ffmpeg/bin/swscale-9.dll"
        "${CMAKE_SOURCE_DIR}/external/ffmpeg/bin/swresample-6.dll"
        
        $<TARGET_FILE_DIR:vk_engine>

    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${CMAKE_CURRENT_SOURCE_DIR}/assets" 
        "$<TARGET_FILE_DIR:vk_engine>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${CMAKE_CURRENT_SOURCE_DIR}/shader" 
        "$<TARGET_FILE_DIR:vk_engine>/shader"

        
)
