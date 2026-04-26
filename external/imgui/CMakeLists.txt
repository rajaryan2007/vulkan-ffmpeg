cmake_minimum_required(VERSION 3.16)

project(ImGui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build ImGui as a STATIC library
add_library(ImGui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp

    imgui.h
    imgui_internal.h
    imconfig.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
)

# Crucial: ImGui is a static library included in VEGA.dll. 
# We need to compile it with __declspec(dllexport) so these symbols are exported out of VEGA.dll 
# for the Sandbox.exe to use.
target_compile_definitions(ImGui PRIVATE IMGUI_API=__declspec\(dllexport\))

target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Windows runtime /MT in Release
if(MSVC)
    target_compile_options(ImGui PRIVATE
        $<$<CONFIG:Release>:/MT>
    )
    # Enable utf-8 for ImGui
    target_compile_options(ImGui PRIVATE /utf-8)
endif()

# Debug symbols
target_compile_options(ImGui PRIVATE
    $<$<CONFIG:Debug>:/Zi>
)

# Release optimization
target_compile_options(ImGui PRIVATE
    $<$<CONFIG:Release>:/O2>
)

# Output ImGui DLL alongside VEGA DLL
set_target_properties(ImGui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${OUTPUT_DIR}/VEGA
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${OUTPUT_DIR}/VEGA
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${OUTPUT_DIR}/VEGA
)