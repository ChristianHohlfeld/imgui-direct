#include "ui/essentials.hpp"
#include "ui/field_detail.hpp"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ui {
namespace {
ImVec4 rgba(Color c){return {c.r,c.g,c.b,c.a};}
ImU32 ink(Color c){return ImGui::GetColorU32(rgba(c));}
float star_size(Size size){
    constexpr float values[]={16.f,20.f,24.f,28.f,32.f};
    return values[std::clamp(static_cast<int>(size),0,4)]*scale();
}
const char* visible_end(const char* value){
    if(!value)return nullptr;
    const char* hidden=std::strstr(value,"##");
    return hidden?hidden:value+std::strlen(value);
}
// Render a recognizable five-point star without concave-polygon dependencies.
void star(ImDrawList* draw,ImVec2 center,float radius,ImU32 fill){
    constexpr float pi=3.14159265358979323846f;
    const float inner=radius*.43f;
    ImVec2 middle[5];
    for(int i=0;i<5;++i){
        const float a=-pi*.5f+(i+.5f)*2.f*pi/5.f;
        middle[i]={center.x+inner*std::cos(a),center.y+inner*std::sin(a)};
    }
    draw->AddConvexPolyFilled(middle,5,fill);
    for(int i=0;i<5;++i){
        const float a=-pi*.5f+i*2.f*pi/5.f;
        const ImVec2 tip{center.x+radius*std::cos(a),center.y+radius*std::sin(a)};
        draw->AddTriangleFilled(middle[(i+4)%5],tip,middle[i],fill);
    }
}
}
bool Link(const char* label,Tone tone,bool disabled,bool hoverOnly){
    if(!label)return false;
    ImGui::PushID(label);
    const char* end=visible_end(label);
    const ImVec2 measured=ImGui::CalcTextSize(label,end);
    const float available=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float h=std::max(measured.y+8.f*scale(),is_touch()?44.f*scale():24.f*scale());
    const float w=std::min(available,measured.x+12.f*scale());
    ImGui::BeginDisabled(disabled);
    const bool hit=ImGui::InvisibleButton("##link",{w,h},ImGuiButtonFlags_EnableNav);
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    const bool hovered=!disabled&&ImGui::GetIO().MouseSource!=ImGuiMouseSource_TouchScreen&&ImGui::IsItemHovered();
    const bool focused=ImGui::IsItemFocused();
    const Color color=hovered?blend(tone_color(tone),palette().baseContent,.15f):
        (tone==Tone::Base?palette().baseContent:tone_color(tone));
    auto* dl=ImGui::GetWindowDrawList();dl->PushClipRect(a,b,true);
    dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
        {a.x+6.f*scale(),a.y+(h-measured.y)*.5f},ink(color),label,end);
    if(!hoverOnly||hovered||focused){
        const float y=a.y+(h+measured.y)*.5f+1.f*scale();
        dl->AddLine({a.x+6.f*scale(),y},{a.x+6.f*scale()+std::min(measured.x,w-12.f*scale()),y},ink(color),scale());
    }
    dl->PopClipRect();
    detail::focus_ring();detail::capture_control();
    ImGui::EndDisabled();ImGui::PopID();
    return hit&&!disabled;
}
bool Segmented(const char* id,std::span<const char* const> labels,int* selected,Options options){
    if(!selected||labels.empty()||labels.size()>64)return false;
    const bool locked=options.state.disabled||options.state.readOnly||options.state.loading;
    const float available=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float h=control_height(options.size),radius=palette().metrics.radiusField*scale();
    const bool vertical=available<labels.size()*64.f*scale();
    const float w=vertical?available:available/static_cast<float>(labels.size());
    auto* draw=ImGui::GetWindowDrawList();
    bool changed=false;
    ImGui::PushID(id?id:"##segments");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha,options.state.readOnly&&!options.state.disabled?1.f:ImGui::GetStyle().DisabledAlpha);
    ImGui::BeginDisabled(locked);
    for(std::size_t i=0;i<labels.size();++i){
        ImGui::PushID(static_cast<int>(i));
        if(i&&!vertical)ImGui::SameLine(0,0);
        const bool clicked=ImGui::InvisibleButton("##segment",{w,h},ImGuiButtonFlags_EnableNav);
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        const bool active=*selected==static_cast<int>(i);
        const bool hovered=!locked&&ImGui::GetIO().MouseSource!=ImGuiMouseSource_TouchScreen&&ImGui::IsItemHovered();
        const bool pressed=!locked&&ImGui::IsItemActive();
        const Color bg=pressed?blend(palette().primary,palette().baseContent,.18f):
            active?palette().primary:
            hovered?blend(palette().base200,palette().primary,.10f):palette().base200;
        const Color fg=pressed||active?palette().primaryContent:palette().baseContent;
        const ImDrawFlags corners=labels.size()==1?ImDrawFlags_RoundCornersAll:
            vertical?(i==0?ImDrawFlags_RoundCornersTop:i+1==labels.size()?ImDrawFlags_RoundCornersBottom:ImDrawFlags_RoundCornersNone):
            (i==0?ImDrawFlags_RoundCornersLeft:i+1==labels.size()?ImDrawFlags_RoundCornersRight:ImDrawFlags_RoundCornersNone);
        draw->AddRectFilled(a,b,ink(bg),radius,corners);
        draw->AddRect(a,b,ink(palette().base300),radius,corners,palette().metrics.border*scale());
        ImGui::PushStyleColor(ImGuiCol_Text,rgba(fg));
        ImGui::RenderTextClipped({a.x+6.f*scale(),a.y},{b.x-6.f*scale(),b.y},
            labels[i]?labels[i]:"",nullptr,nullptr,{.5f,.5f});
        ImGui::PopStyleColor();
        detail::focus_ring();detail::capture_control();
        if(clicked&&!locked&&*selected!=static_cast<int>(i)){*selected=static_cast<int>(i);changed=true;}
        ImGui::PopID();
    }
    ImGui::EndDisabled();ImGui::PopStyleVar(2);ImGui::PopID();
    return changed;
}
bool Rating(const char* id,int* selected,int maxStars,Size size,bool readOnly,bool disabled){
    if(!selected||maxStars<=0||maxStars>10)return false;
    ImGui::PushID(id?id:"##rating");
    if(id&&*id)ImGui::TextUnformatted(id);
    const float glyph=star_size(size),hit=std::max(glyph+8.f*scale(),is_touch()?44.f*scale():glyph);
    const float gap=2.f*scale(),available=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const int columns=std::clamp(static_cast<int>((available+gap)/(hit+gap)),1,maxStars);
    const bool locked=disabled||readOnly;
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha,readOnly&&!disabled?1.f:ImGui::GetStyle().DisabledAlpha);
    ImGui::BeginDisabled(locked);
    bool changed=false;
    for(int i=0;i<maxStars;++i){
        if(i%columns)ImGui::SameLine(0,gap);
        ImGui::PushID(i);
        const bool clicked=ImGui::InvisibleButton("##star",{hit,hit},ImGuiButtonFlags_EnableNav);
        const auto a=ImGui::GetItemRectMin();
        const ImVec2 center{a.x+hit*.5f,a.y+hit*.5f};
        const Color fg=i<*selected?palette().warning:blend(palette().baseContent,palette().base100,.78f);
        star(ImGui::GetWindowDrawList(),center,glyph*.44f,ink(fg));
        detail::focus_ring();detail::capture_control();
        if(clicked&&!locked&&*selected!=i+1){*selected=i+1;changed=true;}
        ImGui::PopID();
    }
    ImGui::EndDisabled();ImGui::PopStyleVar();ImGui::PopID();
    return changed;
}
void ChatBubble(const char* message,bool fromSelf,Tone tone,const char* author,const char* footer){
    message=message?message:"";
    ImGui::PushID(message);
    const auto& pal=palette();
    const Color bg=tone==Tone::Base?pal.base300:tone_color(tone);
    const Color fg=tone==Tone::Base?pal.baseContent:tone_content(tone);
    const float available=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float padX=16.f*scale(),padY=8.f*scale();
    const float maxWidth=std::max(1.f,available*.9f);
    const float wrap=std::max(1.f,maxWidth-padX*2.f);
    if(author&&*author){
        ImGui::PushStyleColor(ImGuiCol_Text,rgba(blend(pal.baseContent,pal.base100,.35f)));
        ImGui::TextUnformatted(author);ImGui::PopStyleColor();
    }
    const auto measured=ImGui::CalcTextSize(message,nullptr,false,wrap);
    const float w=std::min(available,std::max(std::min(40.f*scale(),available),std::min(maxWidth,measured.x+padX*2.f)));
    const float h=std::max(32.f*scale(),measured.y+padY*2.f);
    const auto origin=ImGui::GetCursorScreenPos();
    const float x=origin.x+(fromSelf?std::max(0.f,available-w):0.f);
    ImGui::SetCursorScreenPos({x,origin.y});
    ImGui::Dummy({w,h});
    const ImVec2 a{x,origin.y},b{x+w,origin.y+h};
    const float radius=pal.metrics.radiusField*scale();
    auto* dl=ImGui::GetWindowDrawList();
    dl->AddRectFilled(a,b,ink(bg),radius);
    dl->PushClipRect(a,b,true);
    dl->AddText(ImGui::GetFont(),ImGui::GetFontSize(),{a.x+padX,a.y+padY},ink(fg),message,nullptr,wrap);
    dl->PopClipRect();
    if(footer&&*footer){
        ImGui::PushStyleColor(ImGuiCol_Text,rgba(blend(pal.baseContent,pal.base100,.5f)));
        ImGui::TextUnformatted(footer);
        ImGui::PopStyleColor();
    }
    ImGui::PopID();
}
