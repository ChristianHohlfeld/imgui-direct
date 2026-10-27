cmake_minimum_required(VERSION 3.21)
project(ui_consumer LANGUAGES CXX)
# A host already using ImGui. Pass its pinned source directory to this example.
if(NOT DEFINED IMGUI_SOURCE_DIR)
  message(FATAL_ERROR "Pass -DIMGUI_SOURCE_DIR=/path/to/pinned/imgui")
endif()
add_library(engine_imgui STATIC
  ${IMGUI_SOURCE_DIR}/imgui.cpp ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
  ${IMGUI_SOURCE_DIR}/imgui_tables.cpp ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(engine_imgui PUBLIC ${IMGUI_SOURCE_DIR})
set(UI_IMGUI_TARGET engine_imgui CACHE STRING "" FORCE)
set(UI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(UI_EMBED_FONT OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(../.. ui)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE imgui_direct::ui)
