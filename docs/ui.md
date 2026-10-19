# Dear ImGui / direct UI foundation

**Fixed GUI core:** Dear ImGui 1.93.0. No SDL, Flutter, React, Sokol, web frontend or extra runtime library.

## Principles

The smallest useful UI system needs one source for colors, typography and density,
then a handful of compositions. Components only accept user state as inputs and
return whether it changed. No parallel widget state machine.

* **Theme tokens**: `base100`, `base200`, `base300`, `baseContent`,
  `primary`, `primaryContent`, `secondary`, `secondaryContent`,
  `neutral`, `neutralContent`, `info`, `success`, `warning`, `error`.
* **Built-in looks**: `slateDark` and `slateLight` with the same semantic roles.
* **Theme API**: `ui::set_mode(ui::Mode::Light)`, `ui::set_mode(ui::Mode::Dark)`,
  or `ui::set_custom_palette(myPalette, darkBase)`.
* **Components**: `ui::Heading`, `Muted`, `Divider`, `Badge`, `Alert`,
  `Button`, `Input`, `Checkbox`, `Switch`, `Slider`, `Tabs`,
  `BeginCard`/`EndCard`.
* **State**: user-owned C++ variables. No CSS and no extra framework.
* **Inputs**: built on ImGui buttons, sliders and inputs for focus/keyboard
  accessibility within Dear ImGui's capabilities.
* **Mobile**: touch-aware default dimensions. `app::set_touch` and
  `ui::set_scale` are the only density controls; iOS uses logical points,
  Android reads physical screen density, and the browser uses CSS pixels.

## Example

```cpp
#include "ui/components.hpp"
ui::set_mode(ui::Mode::Dark);
bool enabled = true;
char name[128] = {};
if (ui::BeginCard("settings", 240.f)) {
    ui::Heading("Settings");
    ui::Input("Name", name, sizeof(name), "Your name");
    ui::Switch("Enabled", &enabled);
    if (ui::Button("Save", ui::Variant::Primary)) {
        // Commit the user's own data, no state hidden in the UI library.
    }
}
ui::EndCard();
```

Call `ui::initialize()` after `ImGui::CreateContext()`. Style changes are
global. A theme is a tiny `Palette` value: alter the tokens instead of
manually styling every button. First-party ImGui platform backends stay intact.

## Scope

This is a first-principles component foundation, not daisyUI itself.
Modal dialogs, combo boxes, full accessibility trees, data tables, virtualization,
cross-platform IME completeness and platform-specific navigation can be added
based on real needs instead of prematurely creating a big framework.

## Tests

`ui_theme_test` checks the palette structure in pure C++ at compile time.
Existing gesture and Unicode tests remain. CI builds Windows, Linux, macOS,
Android ARM64, iOS simulator and WebAssembly. Real-device quality remains
a separate acceptance step.
