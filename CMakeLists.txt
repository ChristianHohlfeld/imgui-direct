cmake_minimum_required(VERSION 3.21)
project(imgui_direct LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(UI_BUILD_DEMO "Build platform application" ${PROJECT_IS_TOP_LEVEL})
option(UI_BUILD_TESTS "Build component-library tests" ${PROJECT_IS_TOP_LEVEL})
option(UI_EMBED_FONT "Embed pinned Inter font (requires Python and build-time download)" ${PROJECT_IS_TOP_LEVEL})
set(UI_IMGUI_TARGET "" CACHE STRING "Existing Dear ImGui CMake target, built from the pinned version")
include(FetchContent)
if(UI_IMGUI_TARGET)
  if(NOT TARGET ${UI_IMGUI_TARGET})
    message(FATAL_ERROR "UI_IMGUI_TARGET must name an existing target")
  endif()
  if(UI_BUILD_DEMO)
    message(FATAL_ERROR "External ImGui target requires UI_BUILD_DEMO=OFF; host provides platform backends")
  endif()
  set(UI_CORE_TARGET ${UI_IMGUI_TARGET})
else()
  FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG c1aaac65e896a45f81f47a4d68f9959e0f86c859)
  FetchContent_MakeAvailable(imgui)
  set(IMGUI "${imgui_SOURCE_DIR}")
  add_library(imgui_core STATIC
    "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp"
    "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp")
  target_include_directories(imgui_core PUBLIC "${IMGUI}" "${IMGUI}/backends")
  set(UI_CORE_TARGET imgui_core)
endif()
add_library(ui_foundation STATIC src/ui/theme.cpp src/ui/components.cpp src/ui/extended.cpp src/ui/essentials.cpp)
add_library(imgui_direct::ui ALIAS ui_foundation)
target_compile_features(ui_foundation PUBLIC cxx_std_20)
if(UI_EMBED_FONT)
# Download an immutable, OFL-licensed proportional font at BUILD time.
# Embed it as C++ data: no runtime font asset, web font fetching, or OS-specific
# font paths. Default Dear ImGui font is deliberately NOT used.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(INTER_UPSTREAM "https://raw.githubusercontent.com/google/fonts/e1d6480102fed30739fead0faee463101f892c8f/ofl/inter")
set(INTER_TTF "${CMAKE_CURRENT_BINARY_DIR}/_fonts/Inter.ttf")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_fonts")
if(NOT EXISTS "${INTER_TTF}")
  file(DOWNLOAD "${INTER_UPSTREAM}/Inter%5Bopsz%2Cwght%5D.ttf"
       "${INTER_TTF}" STATUS FONT_STATUS TLS_VERIFY ON)
  list(GET FONT_STATUS 0 FONT_RESULT)
  if(NOT FONT_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not download pinned Inter font: ${FONT_STATUS}")
  endif()
endif()
# Inter's original SIL OFL license is included in licenses/Inter-OFL.txt.
set(INTER_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated/inter_font.cpp")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_font.py" "${INTER_TTF}" "${INTER_GENERATED}"
    RESULT_VARIABLE FONT_CONVERT_RESULT)
if(NOT FONT_CONVERT_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to embed Inter font")
endif()
target_sources(ui_foundation PRIVATE "${INTER_GENERATED}")
target_compile_definitions(ui_foundation PRIVATE UI_EMBED_FONT=1)
endif()
target_include_directories(ui_foundation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(ui_foundation PUBLIC ${UI_CORE_TARGET})
if(UI_BUILD_DEMO)
if(EMSCRIPTEN)
  add_executable(imgui_direct src/app.cpp src/platform_web.cpp
    "${IMGUI}/backends/imgui_impl_opengl3.cpp")
  target_compile_definitions(imgui_direct PRIVATE IMGUI_IMPL_OPENGL_ES3)
  target_link_options(imgui_direct PRIVATE
    "SHELL:-s WASM=1" "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s MAX_WEBGL_VERSION=2" "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
    "SHELL:-s EXPORTED_FUNCTIONS=_main,_mobile_insert_utf8,_mobile_backspace,_mobile_focus,_mobile_control,_gallery_review,_gallery_review_variant"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=ccall")
  set_target_properties(imgui_direct PROPERTIES OUTPUT_NAME index SUFFIX ".html")
elseif(WIN32)
  # Windows GUI subsystem: no console window, while retaining direct Win32/WGL.
  add_executable(imgui_direct WIN32 src/app.cpp src/platform_win32.cpp
    "${IMGUI}/backends/imgui_impl_win32.cpp"
    "${IMGUI}/backends/imgui_impl_opengl3.cpp")
  target_link_libraries(imgui_direct PRIVATE opengl32 user32 gdi32)
elseif(APPLE)
  enable_language(OBJCXX)
  add_executable(imgui_direct MACOSX_BUNDLE src/app.cpp src/platform_apple.mm
    "${IMGUI}/backends/imgui_impl_metal.mm")
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(imgui_direct PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
      XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0")
    target_link_libraries(imgui_direct PRIVATE "-framework UIKit")
  else()
    target_sources(imgui_direct PRIVATE "${IMGUI}/backends/imgui_impl_osx.mm")
    target_link_libraries(imgui_direct PRIVATE "-framework Cocoa" "-framework GameController")
  endif()
  target_link_libraries(imgui_direct PRIVATE "-framework Foundation" "-framework Metal" "-framework MetalKit" "-framework QuartzCore" "-framework CoreGraphics")
elseif(ANDROID)
  set(GLUE "${ANDROID_NDK}/sources/android/native_app_glue")
  add_library(imgui_direct SHARED src/app.cpp src/platform_android.cpp
    "${GLUE}/android_native_app_glue.c"
    "${IMGUI}/backends/imgui_impl_android.cpp"
    "${IMGUI}/backends/imgui_impl_opengl3.cpp")
  target_include_directories(imgui_direct PRIVATE "${GLUE}")
  target_compile_definitions(imgui_direct PRIVATE IMGUI_IMPL_OPENGL_ES3)
  target_link_libraries(imgui_direct PRIVATE android log EGL GLESv3)
elseif(UNIX)
  find_package(X11 REQUIRED)
  find_package(OpenGL REQUIRED)
  add_executable(imgui_direct src/app.cpp src/platform_x11.cpp
    "${IMGUI}/backends/imgui_impl_opengl3.cpp")
  target_link_libraries(imgui_direct PRIVATE X11::X11 OpenGL::GL)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
target_sources(imgui_direct PRIVATE src/state_gallery.cpp src/component_catalog.cpp)
target_link_libraries(imgui_direct PRIVATE ui_foundation)
target_include_directories(imgui_direct PRIVATE src)

endif() # UI_BUILD_DEMO

# Pure C++ deterministic touch tests (no test framework or new dependencies).
include(CTest)
if(UI_BUILD_TESTS AND BUILD_TESTING AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  add_executable(touch_test tests/touch_test.cpp)
  target_include_directories(touch_test PRIVATE src)
  add_test(NAME touch_gestures COMMAND touch_test)
  add_executable(utf_test tests/utf_test.cpp)
  target_include_directories(utf_test PRIVATE src)
  add_test(NAME unicode_roundtrip COMMAND utf_test)
  add_executable(ui_theme_test tests/ui_theme_test.cpp)
  target_include_directories(ui_theme_test PRIVATE src)
  add_test(NAME ui_palette COMMAND ui_theme_test)
  add_executable(ui_component_test tests/ui_component_test.cpp)
  target_include_directories(ui_component_test PRIVATE src)
  target_link_libraries(ui_component_test PRIVATE ui_foundation)
  add_test(NAME ui_component_states COMMAND ui_component_test)
  add_executable(ui_extended_test tests/ui_extended_test.cpp)
  target_link_libraries(ui_extended_test PRIVATE ui_foundation)
  add_test(NAME ui_extended_interactions COMMAND ui_extended_test)
  add_executable(ui_essentials_test tests/ui_essentials_test.cpp)
  target_link_libraries(ui_essentials_test PRIVATE ui_foundation)
  add_test(NAME ui_essentials_states COMMAND ui_essentials_test)
  add_executable(ui_state_test tests/ui_state_test.cpp src/state_gallery.cpp)
  target_link_libraries(ui_state_test PRIVATE ui_foundation)
  add_test(NAME ui_full_state_audit COMMAND ui_state_test)
  # Assertions are the test oracle; keep active in Release, on every compiler.
  foreach(test_target touch_test utf_test ui_theme_test ui_component_test ui_extended_test ui_essentials_test ui_state_test)
    if(MSVC)
      target_compile_options(${test_target} PRIVATE /UNDEBUG)
    else()
      target_compile_options(${test_target} PRIVATE -UNDEBUG)
    endif()
  endforeach()
  add_executable(scroll_test tests/scroll_test.cpp)
  target_include_directories(scroll_test PRIVATE src)
  target_link_libraries(scroll_test PRIVATE ${UI_CORE_TARGET})
  add_test(NAME scroll_pixel_pipeline COMMAND scroll_test)
  if(MSVC)
    target_compile_options(scroll_test PRIVATE /UNDEBUG)
  else()
    target_compile_options(scroll_test PRIVATE -UNDEBUG)
  endif()
endif()
