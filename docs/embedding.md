# Embedding the component library

The library is project-owned C++20 on **Dear ImGui 1.93.0**, pinned to
`c1aaac65e896a45f81f47a4d68f9959e0f86c859`. It has no DaisyUI, Tailwind,
HTML, Emscripten, OS, engine, GPU or window-system dependency. Dear ImGui remains
a required dependency; this is not a replacement renderer. `src/ui/scroll.hpp`
uses version-pinned Dear ImGui internals. Use the same ImGui build, compile
configuration, allocator and context in the host and library.

## CMake, with an engine's existing ImGui target

```cmake
set(UI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(UI_EMBED_FONT OFF CACHE BOOL "" FORCE)
set(UI_IMGUI_TARGET my_imgui CACHE STRING "" FORCE)
add_subdirectory(vendor/imgui-direct)
target_link_libraries(my_engine PRIVATE imgui_direct::ui)
```

`my_imgui` must already exist and expose the pinned ImGui headers. No second
ImGui copy, platform backend, Python or font download is introduced. Without
`UI_IMGUI_TARGET`, CMake fetches and builds the pinned ImGui core. Library-only
builds do not probe X11, OpenGL, Android, Apple or Windows SDKs.
`UI_BUILD_DEMO`, `UI_BUILD_TESTS` and `UI_EMBED_FONT` default off when added as a subdirectory.
The demo defaults to embedded Inter; engines can use their existing font atlas.
Do not point the demo at an external target: the demo owns platform backends.

## Frame contract

```cpp
#include "ui/extended.hpp"
#include "ui/scroll.hpp"

// Once, after the host creates/selects its ImGui context:
ui::initialize();
ui::set_mode(ui::Mode::Dark);

// In the host's UI thread, after backend NewFrame + ImGui::NewFrame:
ui::begin_frame();
ui::scroll::begin_frame();
ui::scroll::dispatch_before_begin(); // only if using the optional pixel adapter
ImGui::Begin("Properties");
ui::Vector3("Position", position);
ui::ColorEdit("Material", rgba);
ui::scroll::apply();
ImGui::End();
ui::scroll::end_frame();
// Host calls ImGui::Render and submits ImDrawData through its own renderer.
```

Use one active UI context on one UI thread. Theme, text-input observer and
optional pixel-scroll adapter currently have process-local state; independent
concurrent themes/contexts are not supported. Call `SetInputObserver(nullptr)`
when its platform adapter is destroyed. This is an explicit current boundary,
not a claim that a renderer-independent API is also thread-safe.

State is supplied by the application: selection indices, expansion state,
split ratios, editable buffers, toast queues. Keep IDs stable across frames;
use `PushID` for repeated rows. `##suffix` hides a control ID from its label.
Strings supplied as content are plain UTF-8 and never interpreted as format strings.
Widgets borrow pointers only for the call, except split ratio (until `EndSplit`)
and input observer registration. Toasts copy their messages on submission.

`BeginCard`, `BeginSplit` and `ImGui::BeginChild` require a matching End **even
when false**. For a split call `NextSplit` exactly once regardless of whether
the first child is visible. Only draw each pane's contents when its Begin/Next
returns true. Dropdowns, context menus, tables, modals and tree nodes require
End **only when Begin returns true**. Always close a fieldset.

## Input, rendering and operating-system services

Feed keyboard, text, mouse, touch and gamepad events through ImGuiIO; do not
invent a second widget focus system. `initialize` enables keyboard navigation.
Hosts supporting gamepads must enable `ImGuiConfigFlags_NavEnableGamepad`,
set `ImGuiBackendFlags_HasGamepad` and send actual gamepad events. Respect
`WantCaptureMouse`, `WantCaptureKeyboard` and `WantTextInput` when routing
engine controls. Keep display size, framebuffer scale and input coordinates
consistent; do not multiply touch positions by DPI twice.

For the optional pixel-scroll path, feed `ui::scroll::add` in logical pixels
**instead of** also sending the same wheel event to ImGui. It is not mandatory;
a normal ImGui backend can use its own wheel path. Integrate `touch.hpp` only
if the host lacks gesture handling. Its scroll outputs are wheel units; the
provided touch adapter converts them to logical pixels once.

Native keyboard/IME, clipboard and file selection remain platform services.
`SetInputObserver` reports visible editable Input/Textarea rectangles and active
state to a host IME adapter; it does not create an OS keyboard. `FileInput`
returns that a request was issued; the supplied host callback must start a
non-blocking picker. Deliver the actual result to host state in a later frame.
No filesystem access, arbitrary URL launch or blocking dialog occurs in the core.
The demo deliberately disables file selection rather than pretending a file was chosen.
Screen-reader accessibility is not implemented by drawing text on a GPU;
platform accessibility bridges remain future integration work.

## Tables, virtual lists and notifications

Tables use ImGui clipping, resizing and sorting metadata. Enable `sortable`
only when your data source implements the requested sort using
`ImGui::TableGetSortSpecs()` and clears `SpecsDirty` after applying it. This
library does not copy/sort engine data or assume its lifetime. Changing column
count/order should change the table ID or clear the corresponding ImGui settings.

`VirtualList` requires fixed-height rows. Include the current ItemSpacing.y
in `rowHeight`, and advance by exactly that height in the callback. Do not
place arbitrary wrapped/variable-height content in the fixed-height clipper.
Only visible rows are submitted; host data remains outside the library.
The 100,000-row native test checks the callback count rather than assuming a
particular CPU or claiming an unsupported frame rate.

`ToastQueue` holds at most eight messages; overflow replaces the oldest.
Supply a monotonic timestamp, in seconds. Duration zero persists until dismissed;
negative/non-finite time/duration and empty messages are rejected. Queue writes
must run on the UI thread; marshal worker notifications there. `update`/`Toasts`
remove expired messages. Strings allocate on submission; ImGui rendering itself
may grow its reusable buffers/font atlas. There is no claim of zero allocations
for a first frame, new glyph, larger table or changed notification text.

## Kern integration boundary

Inspected `ChristianHohlfeld/kernengine.de`: its `kern/build.sh` builds a
**wasi-sdk reactor**, and `kern/src/studio/ui.cpp` currently produces the Studio
command/JSON protocol rendered by the web host. The gallery here is an
**Emscripten** application. Linking its `main`/platform code into Kern would
therefore be incorrect.

A Kern adapter must compile the core plus ImGui with Kern's toolchain, feed
input through the existing host, and consume `ImDrawData` after the scene pass:
vertex/index uploads, texture lifetime (including ImGui 1.93 font texture
updates), scissor rectangles, alpha blending, display scaling, context loss
and restoration of engine GL state. Keep Undo/Redo, asset IDs, actor lifetimes
and commands in Kern. Bind property widgets to command transactions rather
than mutating live engine state without its normal history mechanism.

The new inspector/layout examples demonstrate the shared components without
introducing a dependency on Kern. This change does **not** replace Kern's
Studio UI or claim that its WASI rendering adapter has already been built.
