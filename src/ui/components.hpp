#pragma once
#include "ui/theme.hpp"
#include "imgui.h"
#include <cstddef>

namespace ui {
enum class Variant { Primary, Secondary, Neutral, Outline, Ghost, Error };
enum class Status { Info, Success, Warning, Error };
void Heading(const char* text);
void Muted(const char* text);
void Divider();
void Badge(const char* text,Status status=Status::Info);
void Alert(const char* title,const char* message,Status status=Status::Info);
bool Button(const char* label,Variant variant=Variant::Primary,ImVec2 size={0,0},bool enabled=true);
bool Tabs(const char* id,const char* const* names,int count,int* selected);
bool Input(const char* label,char* buffer,std::size_t capacity,const char* hint="");
bool Checkbox(const char* label,bool* value);
bool Switch(const char* label,bool* value);
bool Slider(const char* label,float* value,float min,float max,const char* fmt="%.2f");
bool BeginCard(const char* id,float height);
void EndCard();
}
