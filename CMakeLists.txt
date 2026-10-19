cmake_minimum_required(VERSION 3.21)
project(imgui_direct LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG c1aaac65e896a45f81f47a4d68f9959e0f86c859)
FetchContent_MakeAvailable(imgui)
set(IMGUI "${imgui_SOURCE_DIR}")
add_library(imgui_core STATIC
  "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp"
  "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp"
  "${IMGUI}/imgui_demo.cpp")
target_include_directories(imgui_core PUBLIC "${IMGUI}" "${IMGUI}/backends")
# Project-owned UI library: semantic tokens and native Dear ImGui components.
add_library(ui_foundation STATIC src/ui/theme.cpp src/ui/components.cpp)
target_include_directories(ui_foundation PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(ui_foundation PUBLIC imgui_core)
if(EMSCRIPTEN)
  add_executable(imgui_direct src/app.cpp src/platform_web.cpp
    "${IMGUI}/backends/imgui_impl_opengl3.cpp")
  target_compile_definitions(imgui_direct PRIVATE IMGUI_IMPL_OPENGL_ES3)
  target_link_options(imgui_direct PRIVATE
    "SHELL:-s WASM=1" "SHELL:-s MIN_WEBGL_VERSION=2"
    "SHELL:-s MAX_WEBGL_VERSION=2" "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
    "SHELL:-s EXPORTED_FUNCTIONS=_main,_mobile_insert_utf8,_mobile_backspace,_mobile_focus"
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
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/ios/Info.plist"
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
target_link_libraries(imgui_direct PRIVATE ui_foundation)
target_include_directories(imgui_direct PRIVATE src)

# Pure C++ deterministic touch tests (no test framework or new dependencies).
include(CTest)
if(BUILD_TESTING AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  add_executable(touch_test tests/touch_test.cpp)
  target_include_directories(touch_test PRIVATE src)
  add_test(NAME touch_gestures COMMAND touch_test)
  add_executable(utf_test tests/utf_test.cpp)
  target_include_directories(utf_test PRIVATE src)
  add_test(NAME unicode_roundtrip COMMAND utf_test)
  add_executable(ui_theme_test tests/ui_theme_test.cpp)
  target_include_directories(ui_theme_test PRIVATE src)
  add_test(NAME ui_palette COMMAND ui_theme_test)
endif()
