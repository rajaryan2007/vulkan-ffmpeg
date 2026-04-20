cmake_minimum_required(VERSION 3.10)
project(vk_engine)


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

find_package(Vulkan REQUIRED)

add_executable(vk_engine
   ${SRC_FILES}
   ${INCLUDE_FILES}
        src/include/demo.hh
        src/demo.cc
 "src/include/miniaudio.h")

set_target_properties(${project_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/assets")

target_include_directories(vk_engine PRIVATE src/include/ external/glm/ external/utils/ external/ffmpeg/include/)

target_link_libraries(vk_engine
    glfw
    Vulkan::Vulkan
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avcodec.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avformat.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/avutil.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/swscale.lib
    ${CMAKE_SOURCE_DIR}/external/ffmpeg/lib/swresample.lib
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
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:vk_engine>/assets"
        "${CMAKE_SOURCE_DIR}/shader"
        "$<TARGET_FILE_DIR:vk_engine>/shader"
)
