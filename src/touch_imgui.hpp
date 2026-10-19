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
            // Touch engine's historical event units were wheel/60; forward
            // them as real logical pixels to the shared scroll coordinator.
            ui::scroll::add(e.value*60.f,e.second*60.f);
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
