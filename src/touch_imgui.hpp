#pragma once
#include "touch.hpp"
#include "imgui.h"
#include "app.hpp"
#include "ui/scroll.hpp"
namespace touch {
inline void dispatch(const Events& events) {
    auto& io=ImGui::GetIO();
    for(const auto& e:events) {
        if(e.type==Type::Zoom) { app::zoom(e.value); continue; }
        if(e.type==Type::Scroll) {
            // Preserve the touch pointer for scroll routing even after
            // releasing a pressed button at an off-screen cancel position.
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(e.x,e.y);
            ui::scroll::add(e.value*60.f,e.second*60.f);
            continue;
        }
        if(e.type==Type::Cancel) {
            // Native touch scrolling cancels a pending click; releasing at
            // the old button location would accidentally activate it.
            io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
            io.AddMouseButtonEvent(0,false);
            continue;
        }
        io.AddMouseSourceEvent(e.pen?ImGuiMouseSource_Pen:ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent(e.x,e.y);
        if(e.type==Type::Down) io.AddMouseButtonEvent(0,true);
        if(e.type==Type::Up) io.AddMouseButtonEvent(0,false);
        if(e.type==Type::LongPress) {
            io.AddMouseButtonEvent(1,true);
            io.AddMouseButtonEvent(1,false);
        }
    }
}
}
