#pragma once
// Private, version-pinned adapter. One state/style path for every native field.
#include "ui/components.hpp"
#include "imgui_internal.h"
#include <algorithm>
#include <cstring>
#include <cmath>
namespace ui::detail {
inline ImVec4 vec(Color c){return {c.r,c.g,c.b,c.a};}
inline ImU32 ink(Color c){return ImGui::GetColorU32(vec(c));}
void capture_control();
inline void focus_ring(){
    if(ImGui::IsItemFocused()&&ImGui::GetIO().NavVisible){
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect({a.x+1,a.y+1},{b.x-1,b.y-1},ink(palette().primary),
            ImGui::GetStyle().FrameRounding,0,2.f*scale());
    }
}
inline void label(const char* value,Field f){
    const char* shown=value?value:"";
    ImGui::PushTextWrapPos();ImGui::TextUnformatted(shown,std::strstr(shown,"##"));ImGui::PopTextWrapPos();
    if(f.required){ImGui::SameLine(0,3*scale());ImGui::TextColored(vec(palette().error),"*");}
}
inline void tooltip_text(const char* begin,const char* end=nullptr){
    if(!begin)return;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,1.f);
    ImGui::PushStyleColor(ImGuiCol_Text,vec(palette().baseContent));
    ImGui::BeginTooltip();
    const float width=std::max(40.f,std::min(360.f*scale(),ImGui::GetIO().DisplaySize.x-24.f*scale()));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+width);ImGui::TextUnformatted(begin,end);ImGui::PopTextWrapPos();
    ImGui::EndTooltip();ImGui::PopStyleColor();ImGui::PopStyleVar();
}
inline void hint(Field f){
    const auto visual=field_visual(f);
    if(f.loading){
        const auto p=ImGui::GetCursorScreenPos();const float h=ImGui::GetTextLineHeight();
        ImGui::Dummy({h,h});const float t=static_cast<float>(ImGui::GetTime()*4);
        auto* d=ImGui::GetWindowDrawList();d->PathArcTo({p.x+h*.5f,p.y+h*.5f},h*.32f,t,t+4.4f,16);
        d->PathStroke(ink(palette().primary),0,1.7f*scale());
        ImGui::SameLine();ImGui::TextUnformatted("Wird verarbeitet …");
    }
    if(f.help&&*f.help){
        const bool feedback=f.invalid||f.validation!=ValidationState::None;
        ImGui::PushStyleColor(ImGuiCol_Text,vec(feedback?visual.border:blend(palette().baseContent,palette().base100,.35f)));
        ImGui::TextWrapped("%s",f.help);ImGui::PopStyleColor();
    }
}
inline void inline_required(Field f){
    if(!f.required)return;
    const auto item=GImGui->LastItemData;
    ImGui::SameLine(0,3.f*scale());ImGui::TextColored(vec(palette().error),"*");
    GImGui->LastItemData=item;
}
inline void field_begin(Field f,bool lockReadOnly=false){
    // Readonly text stays navigable/selectable; non-text controls are locked,
    // with full-opacity readonly styling. Inherited BeginDisabled always wins.
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha,(f.readOnly||f.loading)&&!f.disabled?1.f:ImGui::GetStyle().DisabledAlpha);
    ImGui::BeginDisabled(f.disabled||f.loading||(lockReadOnly&&f.readOnly));
    const auto p=field_visual(f);
    ImGui::PushStyleColor(ImGuiCol_Border,vec(p.border));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,vec(p.normal));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,vec(ImGui::GetIO().MouseSource==ImGuiMouseSource_TouchScreen?p.normal:p.hover));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,vec(p.active));
    ImGui::PushStyleColor(ImGuiCol_CheckMark,vec(p.accent));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,vec(p.accent));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,(f.invalid||f.validation!=ValidationState::None)?palette().metrics.border*1.5f:palette().metrics.border);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{10.f*scale(),std::max(0.f,(control_height(f.size)-ImGui::GetFontSize())*.5f)});
}
inline void field_end(Field f){
    focus_ring();capture_control();
    const ImGuiLastItemData item=GImGui->LastItemData;
    ImGui::PopStyleVar(2);ImGui::PopStyleColor(6);ImGui::EndDisabled();ImGui::PopStyleVar();
    hint(f);
    // Help/validation/busy content must not replace the actual input item.
    GImGui->LastItemData=item;
}
struct FieldScope {
    Field f;
    explicit FieldScope(const char* name,Field field):f(field){
        ImGui::PushID(name);label(name,f);field_begin(f,true);ImGui::SetNextItemWidth(-1);
    }
    ~FieldScope(){field_end(f);ImGui::PopID();}
};
}
