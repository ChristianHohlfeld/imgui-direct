#pragma once
// Self-contained native, immediate-mode essentials for Kern and other hosts.
// Host owns all mutable data and actions. No network, CSS, DOM, or widget tree.
#include "ui/components.hpp"
#include <span>

namespace ui {
// A real underlined keyboard/touch/mouse link. Caller handles navigation/action.
// hoverOnly suppresses the underline until mouse hover or keyboard focus.
bool Link(const char* label,Tone tone=Tone::Primary,bool disabled=false,bool hoverOnly=false);
// A mutually exclusive joined/segmented group; no hidden internal selection.
// Responsive: switches to vertical stack before segments become unusably narrow.
bool Segmented(const char* id,std::span<const char* const> labels,int* selected,Options options={});
// Star selection; 0 = unset; one selectable item per star, keyboard navigable.
// Readonly shows the full-contrast values but cannot edit them.
bool Rating(const char* id,int* selected,int maxStars=5,Size size=Size::MD,
            bool readOnly=false,bool disabled=false);
// Lightweight static daisy-like chat layout; never acts as a chat transport.
// author / footer optional. No application messages are persisted by the library.
void ChatBubble(const char* text,bool fromSelf=false,Tone tone=Tone::Base,
                const char* author=nullptr,const char* footer=nullptr);
}
