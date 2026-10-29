#pragma once
// Shared pixel-accurate scroll input. One wheel/pointer delta means the same
// logical distance on every backend. No global smoothing of OS momentum.
// Self-contained C++20 core; only this adapter needs Dear ImGui.
#include <algorithm>
#include <cmath>
#include "imgui.h"
#include "imgui_internal.h" // Pinned Dear ImGui 1.93: pre-Begin scroll targets avoid a 1-frame delay.
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
    bool activeDrag=false;
    unsigned packets=0, targetChecks=0;
    float lastPointerX=0,lastPointerY=0,lastMaxY=0,lastScrollY=0;
    bool lastInside=false;
    // Elastic visual displacement at scroll boundaries, in logical px.
    // Kept independent of the actual scroll position and nested scroll chain.
    float overscrollY=0.f, springVelocityY=0.f;
    bool elasticInput=false, reduceMotion=false;
    void push_edge(float remaining){
        if(reduceMotion||!std::isfinite(remaining)||remaining==0.f)return;
        // Rubber-band resistance increases with travel; never allow runaway.
        const float resistance=.26f/(1.f+std::abs(overscrollY)/44.f);
        overscrollY=std::clamp(overscrollY+remaining*resistance,-56.f,56.f);
        springVelocityY=0.f;
    }
    void step_spring(float dt){
        if(!std::isfinite(dt)||dt<=0.f)return;
        if(reduceMotion){overscrollY=springVelocityY=0.f;return;}
        dt=std::clamp(dt,0.f,1.f/30.f);
        // Stable critically damped return; independent from OS scroll momentum.
        const float acceleration=-155.f*overscrollY-25.f*springVelocityY;
        springVelocityY+=acceleration*dt;
        overscrollY+=springVelocityY*dt;
        if(std::abs(overscrollY)<.12f&&std::abs(springVelocityY)<1.4f)
            overscrollY=springVelocityY=0.f;
    }
    void add(float right,float down){
        ++packets;
        if(std::isfinite(right))dx=std::clamp(dx+right,-20000.f,20000.f);
        if(std::isfinite(down))dy=std::clamp(dy+down,-20000.f,20000.f);
    }
    void next_frame(){appliedX=appliedY=0;rowsDrawn=0;activeDrag=false;lastInside=false;}
    void discard(){dx=dy=0;}
};
inline Buffer& state(){static Buffer queue;return queue;}
inline void add(float right,float down){state().add(right,down);}
inline void add_elastic(float right,float down){
    state().elasticInput=true;
    state().add(right,down);
}
inline void set_reduce_motion(bool reduced){state().reduceMotion=reduced;}

inline void set_drag_control_active(bool value){state().activeDrag|=value;}
inline bool drag_control_active(){return state().activeDrag;}
inline void add_wheel(float dx,float dy,Unit units,float line=20.f,float page=760.f){
    add_elastic(to_pixels(dx,units,line,page),to_pixels(dy,units,line,page));
}
inline void begin_frame(){
    auto& q=state();q.step_spring(GImGui?ImGui::GetIO().DeltaTime:1.f/60.f);q.next_frame();
}
// MouseWheel must be applied BEFORE Begin() lays out the window this
// frame. Calling SetScrollY() after drawing schedules the next frame and
// produces perceptible lag at 60/120 Hz. Dear ImGui's internal hovered
// window pointer is explicitly version-pinned by CMake and has no new
// runtime dependency. We still retain the public-API fallback below.
inline void dispatch_before_begin(){
    auto& q=state();
    if(q.dy==0.f && q.dx==0.f)return;
    if(!GImGui)return;
    for(ImGuiWindow* win=GImGui->HoveredWindow;win;win=win->ParentWindow){
        if(win->Flags & ImGuiWindowFlags_NoScrollWithMouse)continue;
        if(q.dy!=0.f && win->ScrollMax.y>0.f){
            const float desired=std::clamp(win->Scroll.y+q.dy,0.f,win->ScrollMax.y);
            const float moved=desired-win->Scroll.y;
            if(moved!=0.f) {
                ImGui::SetScrollY(win,desired);
                q.dy-=moved;q.appliedY+=moved;
            }
        }
        if(q.dx!=0.f && win->ScrollMax.x>0.f){
            const float desired=std::clamp(win->Scroll.x+q.dx,0.f,win->ScrollMax.x);
            const float moved=desired-win->Scroll.x;
            if(moved!=0.f){
                ImGui::SetScrollX(win,desired);
                q.dx-=moved;q.appliedX+=moved;
            }
        }
        if(q.dx==0.f && q.dy==0.f)break;
    }
}
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
    const bool inside=std::isfinite(pointer.x)&&std::isfinite(pointer.y)&&
       pointer.x>=origin.x&&pointer.y>=origin.y&&
       pointer.x<origin.x+dimensions.x&&pointer.y<origin.y+dimensions.y;
    auto& q=state();
    q.lastPointerX=pointer.x;q.lastPointerY=pointer.y;
    q.lastMaxY=ImGui::GetScrollMaxY();q.lastScrollY=ImGui::GetScrollY();
    q.lastInside=inside;++q.targetChecks;
    if(!inside)return;
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
// Invoke once at the outermost scrollable child AFTER nested scroll chaining.
inline void bounce_after_scroll(){
    auto& q=state();
    if(!q.lastInside||!q.elasticInput||q.reduceMotion)return;
    if(std::abs(q.dy)<.001f)return;
    // A remaining delta is overscroll only once the scroll chain is saturated.
    // Never intercept real scroll pixels or interfere with scroll position.
    const bool edge=(q.dy<0.f&&q.lastScrollY<=.5f)||
                    (q.dy>0.f&&q.lastScrollY>=q.lastMaxY-.5f);
    if(edge)q.push_edge(q.dy);
}
// Translate only the child CONTENT vertices, not its own background or
// clipping region. The real ImGui scroll position stays clamped and accurate.
// The short-lived displacement is a visual feedback layer, not a scroll engine.
inline void render_bounce(int firstVertex){
    auto& q=state();
    if(q.reduceMotion||std::abs(q.overscrollY)<.1f||!GImGui)return;
    // A browser-like rubber band must move *every child card and its preview*
    // together. An ImGui child has its own draw list, unlike text in the root.
    ImGuiWindow* root=ImGui::GetCurrentWindow();
    ImDrawList* draw=root->DrawList;
    firstVertex=std::clamp(firstVertex,0,draw->VtxBuffer.Size);
    const float displacement=-q.overscrollY;
    for(int i=firstVertex;i<draw->VtxBuffer.Size;++i)
        draw->VtxBuffer[i].pos.y+=displacement;

    // Walk only active native child windows. Popups, tooltips and toasts are
    // floating overlays and must stay fixed. All vertices/clip regions are
    // translated together; the outer page clip remains a hard safety limit.
    for(ImGuiWindow* child : GImGui->Windows){
        if(child==root||!child->Active||!child->DrawList||
           (child->Flags&(ImGuiWindowFlags_Popup|ImGuiWindowFlags_Tooltip)))continue;
        bool descendant=false;
        for(ImGuiWindow* parent=child->ParentWindow;parent;parent=parent->ParentWindow){
            if(parent==root){descendant=true;break;}
        }
        if(!descendant)continue;
        for(ImDrawVert& vertex:child->DrawList->VtxBuffer)
            vertex.pos.y+=displacement;
        for(ImDrawCmd& cmd:child->DrawList->CmdBuffer){
            cmd.ClipRect.y=std::max(root->ClipRect.Min.y,cmd.ClipRect.y+displacement);
            cmd.ClipRect.w=std::min(root->ClipRect.Max.y,cmd.ClipRect.w+displacement);
            if(cmd.ClipRect.w<cmd.ClipRect.y)cmd.ClipRect.w=cmd.ClipRect.y;
        }
    }
}
inline void end_frame(){
    auto& q=state();q.discard();q.elasticInput=false;
}
}
