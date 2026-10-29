#pragma once
// Dear ImGui is the permanent UI core. This is a native C++ component layer,
// not a replacement renderer, styling DSL, CSS runtime, or second state machine.
#include "ui/theme.hpp"
#include "imgui.h"
#include <cstddef>

namespace ui {
enum class Variant { Primary, Secondary, Neutral, Outline, Ghost, Error, Success, Warning, Info, Soft };
enum class Status { Info, Success, Warning, Error };
enum class Size { XS, SM, MD, LG, XL };

struct State {
    bool disabled=false;
    bool loading=false;
    bool selected=false;
    bool invalid=false;
    bool readOnly=false;
};
struct Options {
    Variant variant=Variant::Primary;
    Size size=Size::MD;
    State state={};
    bool fullWidth=false;
};
enum class ValidationState { None, Success, Warning, Error };
struct Field {
    Size size=Size::MD;
    bool disabled=false;
    bool readOnly=false;
    bool invalid=false;
    bool required=false;
    const char* help=nullptr;
    // Appended for source compatibility with existing aggregate initializers.
    bool loading=false;
    ValidationState validation=ValidationState::None;
    bool indeterminate=false; // Checkbox only; caller clears after a change.
};
struct Interaction {
    bool hovered=false; // pointer hover; false on touch after release
    bool held=false; // active press and drag, mouse or touch
    bool focused=false; // keyboard navigation focus
    bool activated=false; // this frame's activation transition
    bool disabled=false;
    bool released=false; // button/key release
};
struct ControlInfo {
    Interaction interaction{};
    ImVec2 minimum{},maximum{};
    ImGuiID id=0;
};
// Most recent library control, captured BEFORE its helper/validation content.
ControlInfo last_control();
struct FieldVisual { Color normal,hover,active,border,accent; };
FieldVisual field_visual(Field field);
struct ButtonVisual {
    Color normal, hover, pressed, text, border;
    bool outline=false;
};
// Theme-derived component states, testable without DOM/CSS.
ButtonVisual button_visual(Options options);

float control_height(Size size=Size::MD);
Interaction last_interaction(bool disabled=false);
// Bounds of the most recent Input()/Textarea(), used by mobile OS/browser IME adapters.
void LastInputBounds(ImVec2& minimum,ImVec2& maximum);
// Optional platform adapter. Called after each visible, editable text field.
// Coordinates are logical screen units. Callback/user must outlive registration.
using InputObserver=void(*)(ImVec2 minimum,ImVec2 maximum,bool active,void* user);
void SetInputObserver(InputObserver observer,void* user=nullptr);
void Title(const char* text);
void Heading(const char* text);
void Muted(const char* text);
void Divider();
void Badge(const char* text,Status status=Status::Info);
void Alert(const char* title,const char* message,Status status=Status::Info);
bool Button(const char* label,Options options,ImVec2 size={0,0});
bool Button(const char* label,Variant variant=Variant::Primary,ImVec2 size={0,0},bool enabled=true);
bool Tabs(const char* id,const char* const* names,int count,int* selected,Size size=Size::MD);
bool Input(const char* label,char* buffer,std::size_t capacity,const char* hint="",Field field={});
bool Textarea(const char* label,char* buffer,std::size_t capacity,Field field={},int rows=4);
bool Select(const char* label,const char* const* items,int count,int* selected,Field field={});
bool Checkbox(const char* label,bool* value,Field field={});
bool Radio(const char* label,int value,int* selected,Field field={});
bool Switch(const char* label,bool* value,Field field={});
bool Slider(const char* label,float* value,float min,float max,const char* fmt="%.2f",Field field={});
void Progress(const char* label,float fraction,Status status=Status::Info);
void Tooltip(const char* description);
void OpenModal(const char* id);
bool BeginModal(const char* id,bool* isOpen=nullptr);
void EndModal();
bool BeginCard(const char* id,float height=0.f);
void EndCard();
}
