#pragma once
#include "tokens.hpp"
namespace ui {
enum class Mode { Dark, Light, Custom };
Mode mode();
const Palette& palette();
void set_mode(Mode mode);
void set_custom_palette(const Palette& custom,bool darkBase);
void initialize();
// Apply queued palette changes before any ImGui widgets are drawn.
void begin_frame();
void set_touch(bool enabled);
bool is_touch();
float scale();
void set_scale(float scale);
void zoom(float ratio);
}
