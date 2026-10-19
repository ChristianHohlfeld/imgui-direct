# imgui-direct

**One C++20 application; Dear ImGui as the permanent UI core.** No SDL, GLFW, Sokol, React or additional GUI/runtime libraries. Direct Win32/WGL/OpenGL, X11/GLX/OpenGL, Apple Cocoa/UIKit/Metal, Android NDK/EGL/GLES3, browser Emscripten/WebGL2/WASM.

Dear ImGui upstream version is pinned to commit `c1aaac65e896a45f81f47a4d68f9959e0f86c859` (v1.93.0). CMake FetchContent downloads the source during first configuration. A system SDK/compiler is still required for each target.

## Windows

Install Visual Studio 2022/2026 with **Desktop development with C++** and CMake tools, and Git. Open a new terminal; `cmake --version` must work. Run **from this repository**, not `C:\Users\chris`:

```powershell
cd $env:USERPROFILE
git clone https://github.com/ChristianHohlfeld/imgui-direct.git
cd imgui-direct
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\imgui_direct.exe
```

For a **ready-built Windows .exe without installing CMake**, open [Actions > Native C++ builds](https://github.com/ChristianHohlfeld/imgui-direct/actions/workflows/native.yml), select the latest successful run, and download the artifact `imgui-direct-windows-x64` at the bottom of the run page. Extract and run the executable.

If `cmake` is not found: `winget install Kitware.CMake`, then restart the terminal. Visual Studio C++ toolchain is also required.

## WebAssembly

Install Emscripten SDK, Ninja, CMake, Git:

```bash
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel
python3 -m http.server 8000 -d build-web
```

Open http://localhost:8000/index.html (do not use file://). [GitHub Actions](https://github.com/ChristianHohlfeld/imgui-direct/actions) builds and attempts to publish to [GitHub Pages](https://christianhohlfeld.github.io/imgui-direct/). Pages source must be set to **GitHub Actions** in Settings > Pages. The web `build` job uploads a standalone `imgui-direct-web-wasm` artifact even if Pages is disabled. The `deploy` job will fail until **Settings > Pages > Build and deployment > Source: GitHub Actions** is enabled. After enabling, re-run the failed deployment job or manually trigger the workflow. Do not consider the demo live until deployment passes.

## Native other systems

- Linux X11: `sudo apt install libx11-dev libgl-dev ninja-build`, then `cmake -S . -B build -G Ninja && cmake --build build`.
- macOS: `cmake -S . -B build -G Xcode && cmake --build build --config Release`.
- iOS simulator: `cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator && cmake --build build-ios --config Release` (requires Xcode and signing settings).
- Android: `cmake -S . -B build-android -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=26 && cmake --build build-android`. The result is a native `.so`; building/installing a signed APK requires Android SDK packaging.

**Limitations:** X11 (not Wayland); mobile text IME/keyboard and browser context restore need work. iOS and Android have not been device-tested. Browser uses Emscripten-generated JavaScript boot code, not a separate HTML UI.
