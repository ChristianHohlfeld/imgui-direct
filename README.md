# imgui-direct

**[Open the interactive component gallery](https://christianhohlfeld.github.io/imgui-direct/)** · [Coverage](docs/component-coverage.md) · [Embed in your engine](docs/embedding.md)

The default gallery opens a **48-component state inspector**: semantic states, real hover/press/focus, light/dark, sizes and button variants. [State audit and verification](docs/state-audit.md).

Eight live C++/WASM example categories: buttons, forms, themes, 100,000-row lists, navigation, data, editor inputs and layouts. The same component library runs natively; GitHub Pages deploys the gallery from `main`.


**One C++20 application; Dear ImGui as the permanent UI core.** No SDL, GLFW, Sokol, React or additional GUI/runtime libraries. Direct Win32/WGL/OpenGL, X11/GLX/OpenGL, Apple Cocoa/UIKit/Metal, Android NDK/EGL/GLES3, browser Emscripten/WebGL2/WASM.

Dear ImGui upstream version is pinned to commit `c1aaac65e896a45f81f47a4d68f9959e0f86c859` (v1.93.0). CMake FetchContent downloads the source during first configuration. A system SDK/compiler is still required for each target.

## Windows

**Native GUI executable:** opens only the ImGui application, without a background console.
**Alt+Enter** toggles borderless, monitor-sized fullscreen and restores the previous
window position/size (including maximized state). No exclusive mode switch,
extra library, or recreated OpenGL context.

Install Visual Studio 2022/2026 with **Desktop development with C++** and CMake tools, and Git. Open a new terminal; `cmake --version` must work. Run **from this repository**, not `C:\Users\chris`:

```powershell
cd $env:USERPROFILE
git clone https://github.com/ChristianHohlfeld/imgui-direct.git
cd imgui-direct
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\imgui_direct.exe
```

For a **ready-built Windows .exe without installing CMake**, open [Actions > Native C++ builds](https://github.com/ChristianHohlfeld/imgui-direct/actions/workflows/native.yml), select the latest successful run, and download the artifact `imgui-direct-windows-x64` at the bottom of the run page. Extract and double-click the executable. **Download the latest successful build**; older artifacts still contain the console window.

If `cmake` is not found: `winget install Kitware.CMake`, then restart the terminal. Visual Studio C++ toolchain is also required.

## WebAssembly

Install Emscripten SDK, Ninja, CMake, Git:

```bash
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel
python3 -m http.server 8000 -d build-web
```

Open http://localhost:8000/index.html (do not use file://). [GitHub Actions](https://github.com/ChristianHohlfeld/imgui-direct/actions) builds and attempts to publish to [GitHub Pages](https://christianhohlfeld.github.io/imgui-direct/). Pages source must be set to **GitHub Actions** in Settings > Pages. The web `build` job uploads a standalone `imgui-direct-web-wasm` artifact even if Pages is disabled. The `deploy` job will fail until **Settings > Pages > Build and deployment > Source: GitHub Actions** is enabled. After enabling, re-run the failed deployment job or manually trigger the workflow. Do not consider the demo live until deployment passes.

## UI foundation (native C++)

The `src/ui/` library is built around Dear ImGui, without any new runtime
dependencies. One semantic `Palette` defines base, content, primary,
secondary, neutral and feedback colors. Built-in *Slate Dark* and *Slate Light*
can be switched live, or replaced with a custom C++ palette.

The standalone `imgui_direct::ui` CMake target can be embedded without platform code or a second ImGui copy. The kit also includes menus, breadcrumbs, pagination, accordion, fieldsets, tables, tree views, virtual lists, split panes, numeric/vector/color/range controls, status, loading, skeletons and bounded toast notifications. See [the complete selection and exclusions](docs/component-coverage.md).

The native component kit includes buttons with focused/hover/pressed/
selected/disabled/loading states, tabs, cards, input, textarea, select,
checkbox, radio, switch, slider, progress, badges, alerts and modals.
It shares five sizes and native touch-first minimum targets.
The public demo is a clean, product-like interface with no backend or debug
information. See [docs/ui.md](docs/ui.md) for examples and component states.

## Mobile input

All touch behavior lives in `src/touch.hpp`, independent of UIKit, Android
and Emscripten. It implements direct manipulation, one-finger swipe to scroll
outside active controls, two-finger scrolling, pinch-to-zoom, long-press context
click and decaying scroll inertia. It intentionally suppresses accidental
clicks when fingers enter/leave multi-touch. Touch targets and typography
scale for mobile, including safe areas and keyboard occlusion.

- **iPad / iPhone:** UIKit delivers native touch/Pencil, pointer hover,
  real keyboard input through an offscreen native UITextView, clipboard via
  UIPasteboard, Metal renderer. Hardware keyboard integration is native OS
  input. No webview.
- **Android:** NDK/NativeActivity/EGL; touch gestures and state stay in C++.
  UI density is read from Android's native AConfiguration and applied once
  through the shared C++ theme; there is no Android-specific widget toolkit.
  A tiny Java platform class supplies Android's required InputConnection/IME
  and system clipboard only (without Gradle or third-party frameworks).
  The signed **debug APK** is a downloadable GitHub Actions artifact.
- **Browser:** WebAssembly/WebGL2; C++ touch engine and a single invisible
  HTML textarea for the mobile browser's native keyboard/IME. This is an
  OS/browser integration adapter, not a second HTML UI.

`ctest --test-dir build -C Release --output-on-failure` validates
gesture transitions on desktop. CI also compiles iOS Simulator, Android ARM64
and WASM. Real iPad/Android/Safari usability, IME candidates and clipboard
permission flows require device testing; builds alone cannot certify that.

## Native other systems

- Linux X11: `sudo apt install libx11-dev libgl-dev ninja-build`, then `cmake -S . -B build -G Ninja && cmake --build build`.
- macOS: `cmake -S . -B build -G Xcode && cmake --build build --config Release`.
- iOS simulator: `cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator && cmake --build build-ios --config Release` (requires Xcode and signing settings).
- Android: `cmake -S . -B build-android -G Ninja -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=26 && cmake --build build-android`. The result is a native `.so`; building/installing a signed APK requires Android SDK packaging.

**Limitations:** Linux X11 (not Wayland); iOS/Android have not been manually device-tested; complex IME composition and web context restoration warrant follow-up QA. Browser uses Emscripten-generated JavaScript boot code, not a separate HTML UI.
