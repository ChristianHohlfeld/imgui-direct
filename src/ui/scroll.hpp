#pragma once
// Shared pixel-accurate scroll input. One wheel/pointer delta means the same
// logical distance on every backend. No global smoothing of OS momentum.
// Self-contained C++20 core; only this adapter needs Dear ImGui.
#include <algorithm>
#include <cmath>
#include "imgui.h"
namespace ui::scroll {
enum class Unit { Pixel=0, Line=1, Page=2 };
inline float to_pixels(float delta, Unit unit, float line=20.f, float page=760.f) {
    if(!std::isfinite(delta))return 0.f;
    const float unitScale=unit==Unit::Pixel?1.f:unit==Unit::Line?line:page;
    return std::clamp(delta*unitScale,-20000.f,20000.f);
}
struct Buffer {
    float dx=0,dy=0,appliedX=0,appliedY=0;
    int rowsDrawn=0;
    void add(float right,float down){
        if(std::isfinite(right))dx=std::clamp(dx+right,-20000.f,20000.f);
        if(std::isfinite(down))dy=std::clamp(dy+down,-20000.f,20000.f);
    }
    void next_frame(){appliedX=appliedY=0;rowsDrawn=0;}
    void discard(){dx=dy=0;}
};
inline Buffer& state(){static Buffer queue;return queue;}
inline void add(float right,float down){state().add(right,down);}
inline void add_wheel(float dx,float dy,Unit units,float line=20.f,float page=760.f){
    add(to_pixels(dx,units,line,page),to_pixels(dy,units,line,page));
}
inline void begin_frame(){state().next_frame();}
// Invoke in the innermost scrollable window first, then the outer viewport.
// Remaining pixels naturally chain to the parent when the child hits its edge.
// Avoids ImGui's 5-lines-per-wheel multiplier, and double interpolation.
inline void apply(){
    // Native trackpads scroll the surface under the pointer, even when a
    // slider, button or text box was focused last frame. IsWindowHovered()
    // can return false for active/overlapped child items and cause lost
    // wheel packets (felt as intermittent scroll lag). Use the actual
    // viewport geometry instead, with innermost-to-parent dispatch order.
    const ImVec2 pointer=ImGui::GetMousePos();
    const ImVec2 origin=ImGui::GetWindowPos();
    const ImVec2 dimensions=ImGui::GetWindowSize();
    if(!std::isfinite(pointer.x)||!std::isfinite(pointer.y)||
       pointer.x<origin.x||pointer.y<origin.y||
       pointer.x>=origin.x+dimensions.x||pointer.y>=origin.y+dimensions.y)
        return;
    auto& q=state();
    if(q.dy!=0.f && ImGui::GetScrollMaxY()>0.f){
        const float current=ImGui::GetScrollY();
        const float desired=std::clamp(current+q.dy,0.f,ImGui::GetScrollMaxY());
        const float moved=desired-current;
        if(moved!=0.f){ImGui::SetScrollY(desired);q.dy-=moved;q.appliedY+=moved;}
    }
    if(q.dx!=0.f && ImGui::GetScrollMaxX()>0.f){
        const float current=ImGui::GetScrollX();
        const float desired=std::clamp(current+q.dx,0.f,ImGui::GetScrollMaxX());
        const float moved=desired-current;
        if(moved!=0.f){ImGui::SetScrollX(desired);q.dx-=moved;q.appliedX+=moved;}
    }
}
inline void end_frame(){state().discard();}
}
