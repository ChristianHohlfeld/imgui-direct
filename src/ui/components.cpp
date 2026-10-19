#include "ui/components.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>

namespace ui {
namespace {
ImVec4 vec(Color c){return {c.r,c.g,c.b,c.a};}
ImU32 packed(Color c){return ImGui::ColorConvertFloat4ToU32(vec(c));}
Color status_color(Status v) {
    const auto& p=palette();
    switch(v){case Status::Success:return p.success;case Status::Warning:return p.warning;
        case Status::Error:return p.error;default:return p.info;}
}
Color intent(Variant v) {
    const auto& p=palette();
    switch(v) {
    case Variant::Primary:return p.primary;
    case Variant::Secondary:return p.secondary;
    case Variant::Neutral:return p.neutral;
    case Variant::Error:return p.error;
    case Variant::Success:return p.success;
    case Variant::Warning:return p.warning;
    case Variant::Info:return p.info;
    default:return p.primary;
    }
}
struct Paint {Color normal,hover,pressed,text;bool outline=false;};
Paint paint(Options options){
    const auto& p=palette();
    const bool subtle=options.variant==Variant::Soft||
                      options.variant==Variant::Ghost||options.variant==Variant::Outline;
    Color base=intent(options.variant);
    Color normal=subtle?p.base200:base;
    if(options.variant==Variant::Ghost)normal=p.base100;
    if(options.state.invalid)base=p.error;
    if(options.state.selected)normal=blend(normal,base,.24f);
    Color fg=(options.variant==Variant::Primary)?p.primaryContent:
             (options.variant==Variant::Secondary)?p.secondaryContent:
             (options.variant==Variant::Neutral)?p.neutralContent:
             (subtle?p.baseContent:p.primaryContent);
    // Use the foreground of the base for feedback buttons only where readable.
    if(options.variant==Variant::Error||options.variant==Variant::Success||
       options.variant==Variant::Warning||options.variant==Variant::Info)
        fg=p.base100;
    if(options.state.invalid){normal=blend(p.base200,p.error,.17f);fg=p.error;}
    Paint b={normal,blend(normal,base,subtle?.16f:.15f),
                  blend(normal,base,subtle?.28f:.29f),fg,
                  options.variant==Variant::Outline};
    if(options.state.selected){b.pressed=blend(b.normal,base,.42f);}
    return b;
}
void focus_ring(){
    if(ImGui::IsItemFocused() && ImGui::GetIO().NavVisible){
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(
            {a.x-2.f,a.y-2.f},{b.x+2.f,b.y+2.f},packed(palette().primary),
            9.f,0,1.6f);
    }
}
void label(const char* text,Field f){
    ImGui::TextUnformatted(text?text:"");
    if(f.required){ImGui::SameLine(0,3);ImGui::TextColored(vec(palette().error),"*");}
}
void hint(Field f){
    if(f.invalid)ImGui::PushStyleColor(ImGuiCol_Text,vec(palette().error));
    else ImGui::PushStyleColor(ImGuiCol_TextDisabled,vec(blend(palette().baseContent,palette().base100,.40f)));
    if(f.help && f.help[0]){
        if(f.invalid)ImGui::TextWrapped("%s",f.help);
        else ImGui::TextDisabled("%s",f.help);
    }
    ImGui::PopStyleColor();
}
void field_begin(Field f){
    if(f.disabled)ImGui::BeginDisabled();
    if(f.invalid) {
        ImGui::PushStyleColor(ImGuiCol_Border,vec(palette().error));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.5f);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Border,vec(palette().base300));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(12.f*scale(),std::max(6.f, (control_height(f.size)-ImGui::GetFontSize())*.37f)));
}
void field_end(Field f){
    focus_ring();
    ImGui::PopStyleVar(2);ImGui::PopStyleColor();
    if(f.disabled)ImGui::EndDisabled();
    hint(f);
}
}
float control_height(Size size){
    const float desktop[]={28.f,33.f,38.f,44.f,50.f};
    const float mobile[]={44.f,44.f,46.f,50.f,58.f};
    int i=std::clamp((int)size,0,4);
    return (is_touch()?mobile[i]:desktop[i])*scale();
}
Interaction last_interaction(bool disabled) {
    return {ImGui::IsItemHovered(),ImGui::IsItemActive(),ImGui::IsItemFocused(),
            ImGui::IsItemActivated(),disabled};
}
void Title(const char* value){
    ImGui::PushStyleColor(ImGuiCol_Text,vec(palette().baseContent));
    ImGui::PushFont(nullptr,25.f);
    ImGui::TextUnformatted(value?value:"");
    ImGui::PopFont();
    ImGui::PopStyleColor();
}
void Heading(const char* value){
    ImGui::PushStyleColor(ImGuiCol_Text,vec(palette().baseContent));
    ImGui::SetWindowFontScale(1.23f);
    ImGui::TextUnformatted(value?value:"");
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleColor();
}
void Muted(const char* value){
    ImGui::PushStyleColor(ImGuiCol_Text,vec(blend(palette().baseContent,palette().base100,.39f)));
    ImGui::TextWrapped("%s",value?value:"");
    ImGui::PopStyleColor();
}
void Divider(){ImGui::Separator();}
void Badge(const char* value,Status status){
    const Color c=status_color(status);
    const ImVec2 measure=ImGui::CalcTextSize(value);
    ImVec2 size{measure.x+18.f,measure.y+7.f};
    const ImVec2 p=ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    ImGui::GetWindowDrawList()->AddRectFilled(p,{p.x+size.x,p.y+size.y},
                 packed(blend(palette().base200,c,.15f)),size.y*.45f);
    ImGui::GetWindowDrawList()->AddText({p.x+9.f,p.y+3.f},packed(c),value);
}
void Alert(const char* title,const char* message,Status status){
    const Color c=status_color(status);
    ImGui::PushID(title);
    const float w=std::max(150.f,ImGui::GetContentRegionAvail().x);
    const ImVec2 measured=ImGui::CalcTextSize(message,nullptr,false,w-35.f);
    const float h=std::max(78.f,measured.y+58.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,vec(blend(palette().base200,c,.07f)));
    ImGui::PushStyleColor(ImGuiCol_Border,vec(blend(palette().base300,c,.35f)));
    ImGui::BeginChild("##notice",{0,h},ImGuiChildFlags_Borders);
    ImGui::PushStyleColor(ImGuiCol_Text,vec(c));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();
    ImGui::TextWrapped("%s",message);
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopID();
}
bool Button(const char* title,Options opt,ImVec2 size){
    const bool disabled=opt.state.disabled||opt.state.loading||opt.state.readOnly;
    const Paint p=paint(opt);
    const float h=control_height(opt.size);
    if(size.y<=0.f||size.y<h)size.y=h;
    if(opt.state.loading && size.x<=0.f)
        size.x=ImGui::CalcTextSize(title).x+ImGui::GetStyle().FramePadding.x*2.f+30.f*scale();
    if(opt.fullWidth)size.x=std::max(0.f,ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleColor(ImGuiCol_Button,vec(p.normal));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,vec(p.hover));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,vec(p.pressed));
    ImGui::PushStyleColor(ImGuiCol_Text,vec(p.text));
    if(p.outline) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,1.f);
        ImGui::PushStyleColor(ImGuiCol_Border,vec(palette().base300));
    }
    if(disabled)ImGui::BeginDisabled();
    const bool clicked=ImGui::Button(title,size) && !disabled;
    if(!disabled)focus_ring();
    if(opt.state.loading){
        // GPU-native animated busy indicator, no image and no extra dependency.
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        const float cx=b.x-17.f*scale(),cy=(a.y+b.y)*.5f,rad=5.5f*scale();
        auto* d=ImGui::GetWindowDrawList();
        const float start=(float)ImGui::GetTime()*3.8f;
        d->PathClear();
        d->PathArcTo({cx,cy},rad,start,start+4.2f,15);
        d->PathStroke(packed(p.text),0,1.8f*scale());
    }
    if(disabled)ImGui::EndDisabled();
    if(p.outline){ImGui::PopStyleColor();ImGui::PopStyleVar();}
    ImGui::PopStyleColor(4);
    return clicked;
}
bool Button(const char* label,Variant variant,ImVec2 size,bool enabled){
    Options o;o.variant=variant;o.state.disabled=!enabled;return Button(label,o,size);
}
bool Tabs(const char* id,const char* const* labels,int count,int* selected,Size size){
    if(!labels||!selected||count<=0)return false;
    const float gap=ImGui::GetStyle().ItemSpacing.x;
    const float w=ImGui::GetContentRegionAvail().x;
    const float per=(w-gap*(count-1))/count;
    if(per<45.f)return false;
    bool changed=false;
    ImGui::PushID(id);
    for(int i=0;i<count;i++){
        ImGui::PushID(i);
        if(i)ImGui::SameLine();
        Options opt;opt.size=size;opt.variant=*selected==i?Variant::Soft:Variant::Ghost;
        opt.state.selected=*selected==i;
        if(Button(labels[i],opt,{per,0})&&*selected!=i){*selected=i;changed=true;}
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}
ImVec2 lastInputMin{},lastInputMax{};
void LastInputBounds(ImVec2& minimum,ImVec2& maximum){
    minimum=lastInputMin;maximum=lastInputMax;
}
bool Input(const char* name,char* data,std::size_t capacity,const char* placeholder,Field field){
    if(!data||capacity==0)return false;
    ImGui::PushID(name);label(name,field);
    field_begin(field);
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed=ImGui::InputTextWithHint("##value",placeholder?placeholder:"",data,capacity,
            field.readOnly?ImGuiInputTextFlags_ReadOnly:0);
    lastInputMin=ImGui::GetItemRectMin();lastInputMax=ImGui::GetItemRectMax();
    field_end(field);
    ImGui::PopID();
    return changed;
}
bool Textarea(const char* name,char* data,std::size_t capacity,Field field,int rows){
    if(!data||capacity==0)return false;
    ImGui::PushID(name);label(name,field);
    field_begin(field);
    const float h=std::max(control_height(field.size)*2.f,
             (ImGui::GetTextLineHeightWithSpacing()*std::max(2,rows))+14.f);
    const bool changed=ImGui::InputTextMultiline("##multiline",data,capacity,{-FLT_MIN,h},
        field.readOnly?ImGuiInputTextFlags_ReadOnly:0);
    field_end(field);ImGui::PopID();return changed;
}
bool Select(const char* name,const char* const* choices,int count,int* selection,Field field){
    if(!choices||!selection||count<=0)return false;
    ImGui::PushID(name);label(name,field);
    field_begin(field);
    if(field.readOnly)ImGui::BeginDisabled();
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool valid=*selection>=0&&*selection<count;
    bool changed=false;
    if(ImGui::BeginCombo("##select",valid?choices[*selection]:"Choose an option")) {
        for(int i=0;i<count;i++){
            bool chosen=i==*selection;
            if(ImGui::Selectable(choices[i],chosen)&&!field.readOnly){
                *selection=i;changed=true;
            }
            if(chosen)ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if(field.readOnly)ImGui::EndDisabled();
    field_end(field);
    ImGui::PopID();
    return changed;
}
bool Checkbox(const char* label,bool* value,Field field) {
    if(!value)return false;
    if(field.disabled||field.readOnly)ImGui::BeginDisabled();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{9.f*scale(),9.f*scale()});
    bool changed=ImGui::Checkbox(label,value);
    if(!field.disabled)focus_ring();
    ImGui::PopStyleVar();
    if(field.disabled||field.readOnly)ImGui::EndDisabled();
    hint(field);return changed;
}
bool Radio(const char* label,int item,int* selected,Field field){
    if(!selected)return false;
    if(field.disabled)ImGui::BeginDisabled();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{9.f*scale(),9.f*scale()});
    const bool changed=ImGui::RadioButton(label,*selected==item)&&!field.readOnly;
    if(changed)*selected=item;
    if(!field.disabled)focus_ring();
    ImGui::PopStyleVar();
    if(field.disabled||field.readOnly)ImGui::EndDisabled();
    hint(field);return changed;
}
bool Switch(const char* label,bool* value,Field field) {
    if(!value)return false;
    ImGui::PushID(label);
    const float w=48.f*scale(),h=std::max(control_height(field.size),42.f*scale());
    // InvisibleButton is a real ImGui nav/input item: it responds to keyboard,
    // mouse and synthesized native touch click. Hover is never required.
    if(field.disabled||field.readOnly)ImGui::BeginDisabled();
    const bool clicked=ImGui::InvisibleButton("##switch",{w,h})&&
                       !field.disabled&&!field.readOnly;
    if(clicked)*value=!*value;
    const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    const float trackH=23.f*scale(),r=trackH*.5f;
    const float cy=(a.y+b.y)*.5f;
    const bool hovered=ImGui::IsItemHovered();
    const bool held=ImGui::IsItemActive();
    Color bg=field.invalid?palette().error:(*value?palette().primary:palette().base300);
    if(hovered)bg=blend(bg,palette().baseContent,.12f);
    if(held)bg=blend(bg,palette().baseContent,.22f);
    if(field.disabled)bg=blend(bg,palette().base200,.48f);
    ImDrawList* d=ImGui::GetWindowDrawList();
    d->AddRectFilled({a.x,cy-r},{a.x+w,cy+r},packed(bg),r);
    const float knobX=a.x+(*value?w-r:r);
    d->AddCircleFilled({knobX,cy},r-3.f*scale(),packed(*value?palette().primaryContent:palette().baseContent),24);
    if(!(field.disabled||field.readOnly))focus_ring();
    if(field.disabled||field.readOnly)ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::PopID();
    hint(field);
    return clicked;
}
bool Slider(const char* name,float* value,float min,float max,const char* fmt,Field field){
    if(!value)return false;
    ImGui::PushID(name);label(name,field);
    field_begin(field);
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool changed=ImGui::SliderFloat("##slider",value,min,max,fmt,
                    field.readOnly?ImGuiSliderFlags_NoInput:0) && !field.readOnly;
    field_end(field);ImGui::PopID();return changed;
}
void Progress(const char* title,float fraction,Status status){
    ImGui::PushID(title);
    ImGui::TextUnformatted(title);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,vec(status_color(status)));
    ImGui::ProgressBar(std::clamp(fraction,0.f,1.f),{-FLT_MIN,11.f*scale()},"");
    ImGui::PopStyleColor();
    ImGui::PopID();
}
void Tooltip(const char* info){
    if(info&&ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s",info);
}
void OpenModal(const char* id){ImGui::OpenPopup(id);}
bool BeginModal(const char* id,bool* open){
    ImGui::SetNextWindowSizeConstraints({280.f,0.f},{640.f,FLT_MAX});
    return ImGui::BeginPopupModal(id,open,ImGuiWindowFlags_AlwaysAutoResize);
}
void EndModal(){ImGui::EndPopup();}
bool BeginCard(const char* id,float height){
    ImGui::PushStyleColor(ImGuiCol_ChildBg,vec(palette().base200));
    ImGui::PushStyleColor(ImGuiCol_Border,vec(palette().base300));
    ImGuiChildFlags flags=ImGuiChildFlags_Borders;
    if(height<=0.f)flags|=ImGuiChildFlags_AutoResizeY;
    return ImGui::BeginChild(id,{0,height<=0?0:height},flags);
}
void EndCard(){ImGui::EndChild();ImGui::PopStyleColor(2);}
}
