#include "ui/components.hpp"
#include <algorithm>
#include <cfloat>

namespace ui {
namespace {
ImVec4 rgba(Color c){return {c.r,c.g,c.b,c.a};}
ImU32 packed(Color c){return ImGui::ColorConvertFloat4ToU32(rgba(c));}
Color status_color(Status type){
    const auto& p=palette();
    switch(type) {
        case Status::Success:return p.success;
        case Status::Warning:return p.warning;
        case Status::Error:return p.error;
        default:return p.info;
    }
}
struct ButtonPaint {
    Color bg,fg,hover,active;
    bool outline=false;
};
ButtonPaint button_paint(Variant type) {
    const auto& p=palette();
    switch(type){
    case Variant::Primary:return {p.primary,p.primaryContent,blend(p.primary,p.baseContent,.12f),blend(p.primary,p.base100,.17f)};
    case Variant::Secondary:return {p.secondary,p.secondaryContent,blend(p.secondary,p.primary,.12f),blend(p.secondary,p.primary,.23f)};
    case Variant::Neutral:return {p.neutral,p.neutralContent,blend(p.neutral,p.primary,.18f),blend(p.neutral,p.base100,.15f)};
    case Variant::Error:return {p.error,p.primaryContent,blend(p.error,p.baseContent,.12f),blend(p.error,p.base100,.15f)};
    case Variant::Outline:return {p.base200,p.baseContent,blend(p.base200,p.primary,.12f),blend(p.base200,p.primary,.22f),true};
    default:return {p.base200,p.baseContent,blend(p.base200,p.primary,.12f),blend(p.base200,p.primary,.18f)};
    }
}
}
void Heading(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text,rgba(palette().baseContent));
    ImGui::SetWindowFontScale(1.16f);
    ImGui::TextUnformatted(text);
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();
}
void Muted(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text,rgba(blend(palette().baseContent,palette().base100,.4f)));
    ImGui::TextWrapped("%s",text);
    ImGui::PopStyleColor();
}
void Divider(){ImGui::Separator();}
void Badge(const char* text,Status status) {
    const auto color=status_color(status);
    const auto size=ImGui::CalcTextSize(text);
    ImVec2 outer(size.x+19.f,size.y+9.f);
    ImVec2 pos=ImGui::GetCursorScreenPos();
    ImGui::Dummy(outer);
    auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(pos,ImVec2(pos.x+outer.x,pos.y+outer.y),
        packed(blend(palette().base200,color,.16f)),outer.y*.46f);
    d->AddText(ImVec2(pos.x+9.5f,pos.y+4.5f),packed(color),text);
}
void Alert(const char* title,const char* message,Status status) {
    const auto color=status_color(status);
    ImGui::PushID(message);
    const float width=ImGui::GetContentRegionAvail().x;
    const float wrapped=std::max(60.f,width-36.f);
    ImVec2 messageSize=ImGui::CalcTextSize(message,nullptr,false,wrapped);
    const float height=std::max(70.f,messageSize.y+49.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,rgba(blend(palette().base200,color,.055f)));
    ImGui::PushStyleColor(ImGuiCol_Border,rgba(blend(palette().base300,color,.35f)));
    ImGui::BeginChild("##alert",ImVec2(0.f,height),ImGuiChildFlags_Borders);
    ImGui::PushStyleColor(ImGuiCol_Text,rgba(color));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::TextWrapped("%s",message);
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopID();
}
bool Button(const char* label,Variant variant,ImVec2 size,bool enabled) {
    auto p=button_paint(variant);
    ImGui::PushStyleColor(ImGuiCol_Button,rgba(p.bg));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,rgba(p.hover));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,rgba(p.active));
    ImGui::PushStyleColor(ImGuiCol_Text,rgba(p.fg));
    if(p.outline) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
        ImGui::PushStyleColor(ImGuiCol_Border,rgba(palette().base300));
    }
    const float minHeight=is_touch()?48.f:38.f;
    if(size.y<=0.f)size.y=minHeight*scale();
    else size.y=std::max(size.y,minHeight*scale());
    if(!enabled)ImGui::BeginDisabled();
    bool clicked=ImGui::Button(label,size);
    if(!enabled)ImGui::EndDisabled();
    if(p.outline){ImGui::PopStyleColor();ImGui::PopStyleVar();}
    ImGui::PopStyleColor(4);
    return clicked;
}
bool Tabs(const char* id,const char* const* labels,int count,int* selected) {
    if(!labels || !selected || count<=0)return false;
    bool changed=false;
    ImGui::PushID(id);
    const float gap=ImGui::GetStyle().ItemSpacing.x;
    const float itemW=std::max(45.f,(ImGui::GetContentRegionAvail().x-gap*(count-1))/count);
    for(int i=0;i<count;++i) {
        ImGui::PushID(i);
        if(i)ImGui::SameLine();
        if(Button(labels[i],i==*selected?Variant::Primary:Variant::Ghost,{itemW,0})) {
            if(*selected!=i){*selected=i;changed=true;}
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}
bool Input(const char* label,char* buffer,std::size_t capacity,const char* hint) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool result=ImGui::InputTextWithHint("##input",hint?hint:"",buffer,capacity);
    ImGui::PopID();
    return result;
}
bool Checkbox(const char* label,bool* value){return ImGui::Checkbox(label,value);}
bool Switch(const char* label,bool* value) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    const char* text=*value?"ON##switch":"OFF##switch";
    bool changed=Button(text,*value?Variant::Primary:Variant::Outline,{is_touch()?82.f:68.f,0});
    if(changed)*value=!*value;
    ImGui::PopID();
    return changed;
}
bool Slider(const char* label,float* value,float min,float max,const char* fmt) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed=ImGui::SliderFloat("##slider",value,min,max,fmt);
    ImGui::PopID();
    return changed;
}
bool BeginCard(const char* id,float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg,rgba(palette().base200));
    ImGui::PushStyleColor(ImGuiCol_Border,rgba(palette().base300));
    return ImGui::BeginChild(id,ImVec2(0.f,height),ImGuiChildFlags_Borders);
}
void EndCard(){ImGui::EndChild();ImGui::PopStyleColor(2);}
}
