# UI Components · Native C++ / Dear ImGui

A compact, reusable native component library, **inspired by daisyUI**, with no CSS,
DOM, browser UI, new GUI engine, or external dependencies beyond Dear ImGui.

## Surface

| Component | API | Built-in interaction |
|---|---|---|
| Button | `ui::Button(label, Options)` | default, hover, pressed, focused, selected, disabled, loading; keyboard/touch |
| Tabs | `ui::Tabs(...)` | selected, hover, focus; thumb-friendly targets |
| Input | `ui::Input(..., Field)` | hover, focus, read-only, disabled, invalid, required, help |
| Textarea | `ui::Textarea(..., Field)` | multiline, keyboard/IME, focus, invalid |
| Select | `ui::Select(..., Field)` | selected, disabled, read-only, keyboard |
| Checkbox | `ui::Checkbox(..., Field)` | checked, unchecked, focus, disabled |
| Radio | `ui::Radio(..., Field)` | single selected option, focus, disabled |
| Switch | `ui::Switch(..., Field)` | on/off, focus, disabled, touch |
| Slider | `ui::Slider(..., Field)` | dragging, keyboard, hover, disabled |
| Progress | `ui::Progress(...)` | semantic progress/status |
| Badge / Alert | `ui::Badge`, `ui::Alert` | semantic info, success, warning, error |
| Card / Modal | `ui::BeginCard`, `ui::BeginModal` | nested content, centered dialog |
| Heading / Muted / Divider / Tooltip | corresponding methods | shared typography and feedback |

**Styles:** primary, secondary, neutral, outline, ghost, soft, info,
success, warning, error. **Sizes:** XS, SM, MD, LG, XL.
**States:** disabled, loading, selected, invalid, readOnly and native input
states hover, active/held and keyboard focus.

Button example:

```cpp
#include "ui/components.hpp"
ui::Options action;
action.variant=ui::Variant::Primary;
action.size=ui::Size::LG;
action.fullWidth=true;
action.state.loading=busy;
if(ui::Button("Speichern", action) && !busy) save();
```

Field example:

```cpp
char name[128] = {};
ui::Field f;
f.required = true;
f.invalid = invalidName;
f.help = invalidName ? "Bitte Namen eingeben" : nullptr;
ui::Input("Name",name,sizeof(name),"Name",f);
```

## Themes

One palette of semantic tokens. Built-in `Slate Dark` and `Slate Light`
share component definitions. Custom themes are data, not another stylesheet:

```cpp
ui::Palette custom = ui::slateLight;
custom.primary = ui::hex(0x7C3AED);
ui::set_custom_palette(custom,false);
```

Themes update on the next frame boundary to preserve ImGui's style stack.
Use `ui::set_touch(true)` once in the OS input adapter; `Size::XS` still
has at least 44 logical points of touch height, `Size::MD` at least 48.
Android reads the display density, iOS uses UIKit logical points, and
WASM uses browser CSS pixels. Pinch zoom scales the same theme metrics.

Keyboard navigation and focus come from ImGui's native input system, not a
second event loop. Touch can activate every actionable component without
hover; hover is a *mouse/trackpad enhancement* only. This does **not** claim
native iOS VoiceOver / Android TalkBack accessibility semantics: canvas-backed
Dear ImGui does not provide an OS accessibility tree out of the box.

## Demonstration

The application now shows a calm, product-like interface with
`Übersicht`, `Elemente` and `Design`. No built-in ImGui example windows,
FPS counters, graphics-debug panels, backend badges, or technical UI text.

CI validates the theme/state dimensions plus the existing touch/Unicode suite
on native targets and compiles browser WebGL2/WASM and Android APK builds.
Actual hardware acceptance remains separately necessary.

## Typography and high-density screens

The project embeds the OFL-licensed **Inter** proportional typeface at build time
(pinned to Google Fonts commit `e1d6480102fed30739fead0faee463101f892c8f`).
Only the final binary contains font data. No runtime request to Google Fonts,
no platform-installed font dependency, no extra UI renderer.

We deliberately do **not** use ProggyClean/ProggyForever, the default ImGui
developer fonts. Responsive layout is in logical/CSS pixels, while the WebGL2
canvas is backed by real device pixel dimensions. Dear ImGui 1.93 renders
fonts dynamically at requested sizes; avoid magnifying a 13px raster bitmap.

Visual QA is captured using headless Chromium WebGL2 at 1× desktop and 2×
phone/iPad resolutions. Reference screenshots from daisyUI button/card docs
are saved alongside actual app captures in the CI visual artifact.

Inter's complete SIL Open Font License is shipped in
[`licenses/Inter-OFL.txt`](../licenses/Inter-OFL.txt).

### Physical pixel invariant (WebGL2)

The native canvas is sized using a `ResizeObserver` with
`device-pixel-content-box` when the browser exposes it. That gives the actual
**physical pixel count of the canvas's content box**. The JS platform adapter
owns the `canvas.width`/`canvas.height` backing store. Every C++ frame reads
its logical CSS size and the real WebGL `drawingBufferWidth/Height`; the
frame is skipped rather than rendering into a mismatched buffer. ImGui uses
`DisplaySize=CSS pixels` and `DisplayFramebufferScale=actual backing/CSS`, so
1.92+ dynamic font rasterization scales into the actual framebuffer (without a
second manual DPI multiplier).

Chromium provides exact device-pixel-content-box sizes; for unsupported
Safari/Firefox configurations we must fall back to rounded CSS size × DPR.
That fallback cannot guarantee compositor-perfect snapping under fractional
DPR, and the operating system/browser can still apply display transforms.
No canvas-based renderer can promise identical text hinting/subpixel-AA to
native DOM text across all display pipelines.

## Scrolling and 100k-row lists

Native trackpad/wheel **pixel events are applied directly to the intended
scrollable ImGui viewport** (no multiply-by-font-size wheel conversion, no
extra smoothing or second momentum). DOM `deltaMode` explicitly handles
pixels/lines/pages. AppKit precise scroll events and macOS native momentum
phases are used as-is; only touch-finger flicks get a single, frame-rate-
invariant exponential release curve. The same `src/ui/scroll.hpp` provides
pixel accumulation, nested scroll chaining, and tests.

`Liste` in the gallery demonstrates `ImGuiListClipper` over 100,000 rows.
The app submits only visible rows; the list is not an infinite network feed,
but shows how endless or paginated data can remain constant-cost per frame.
A real device and trackpad are required to compare subjective OS kinetics.

## Bootstrap and interaction guarantees

The WebAssembly entry loads automatically. Before the module starts, the
browser can display only a **noninteractive** status indicator; a C++/Dear ImGui
button cannot exist until the WASM runtime is initialized. There is no second
HTML start button with different behavior. After initialization, every action
uses `ui::Button` from the C++ component library.

`ui::button_visual(options)` exposes normal/hover/pressed/foreground/border
color roles for deterministic tests. Solid buttons have **distinct** pressed
and hover shades, selected buttons have a semantic outline, invalid controls
use error outline, disabled/loading cannot activate, and touch uses pressed
feedback rather than sticky hover. Keyboard focus uses an accessible visible
ring when keyboard navigation is active. Gesture cancel prevents a touch
scroll from committing a pending button click.

## Native input invariants

Every scroll event uses **logical pixels** inside the shared C++ touch/scroll
engine. OS/browser trackpad momentum passes through unchanged. Finger kinetic
momentum is integrated analytically, once, at the render frame cadence.

An OS `touchcancel`, interrupted gesture, app background, or browser blur
cancels pending primary-button interaction and kinetic scrolling. Cancellation
is never treated as a regular tap/release. Long-press likewise cancels the
primary-button press before raising a context event.

Buttons, switches, radio buttons, checkboxes, combos and list selections
request the native hand cursor on mouse hover, not on touch; disabled widgets
do not request it. The WebGL canvas reflects that cursor without any HTML
component clone. This keeps the platform widget interaction rules shared.
