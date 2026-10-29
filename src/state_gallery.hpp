#pragma once
#include "ui/extended.hpp"
#include <cstdint>
namespace review {
enum class State { Normal, Selected, Disabled, ReadOnly, Loading, Error, Warning, Success, Empty, Mixed, Open, Closed, Minimum, Maximum, LongText, Required, Count };
constexpr std::uint32_t bit(State s){return 1u<<static_cast<unsigned>(s);}
struct Component {const char* name;std::uint32_t states;bool interactive;};
int count();
const Component& component(int index);
const char* state_name(State state);
bool supports(int index,State state);
void select(int component,State state);
int selected_component();
State selected_state();
int changes();
ui::ControlInfo primary();
void preview(); // Real components; pointer/key interactions are never painted simulations.
void draw();
void set_size(ui::Size size);
void set_variant(ui::Variant variant);
}
