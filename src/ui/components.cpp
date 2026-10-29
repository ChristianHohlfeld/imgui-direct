#include "ui/components.hpp"
#include "ui/scroll.hpp"
#include "ui/field_detail.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstring>

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
using Paint=ButtonVisual;
Color lighten(Color c,float ratio){return blend(c,hex(0xffffff),ratio);}
Color darken(Color c,float ratio){return blend(c,hex(0x020617),ratio);}

Paint paint(Options options){
    const Palette& p=palette();
    const bool ghost=options.variant==Variant::Ghost;
    const bool outline=options.variant==Variant::Outline;
    const bool soft=options.variant==Variant::Soft;
    const bool subtle=ghost||outline||soft;
    const Color base=intent(options.variant);
    const Color content=(options.variant==Variant::Primary)?p.primaryContent:
        (options.variant==Variant::Secondary)?p.secondaryContent:
        (options.variant==Variant::Neutral)?p.neutralContent:
        (options.variant==Variant::Info)?p.infoContent:
        (options.variant==Variant::Success)?p.successContent:
        (options.variant==Variant::Warning)?p.warningContent:
        (options.variant==Variant::Error)?p.errorContent:p.baseContent;
    // Converting a solid background into itself cannot produce a hover or
    // pressed state. Differentiate them in the same semantic color scale.
    Color normal=subtle?(ghost||outline?Color{p.base100.r,p.base100.g,p.base100.b,0.f}:blend(p.base200,base,.12f)):base;
    Color text=subtle?(soft?base:p.baseContent):content;
    Color hover=subtle?blend(p.base100,base,ghost?.12f:.19f):lighten(base,.16f);
    // A near-black neutral cannot get meaningful pressed contrast by darkening
    // further. Press it toward white instead, distinctly beyond its hover.
    Color pressed=subtle?blend(p.base100,base,.30f):
        (base.r+base.g+base.b<.45f?lighten(base,.34f):darken(base,.22f));
    if(options.state.selected){
        normal=blend(normal,base,subtle?.23f:.10f);
        hover=blend(normal,base,.28f);
        pressed=blend(normal,base,.43f);
    }
    Color border=(outline||options.state.selected)?base:p.base300;
    if(options.state.invalid){
        border=p.error;
        normal=blend(p.base200,p.error,.10f);
        hover=blend(normal,p.error,.17f);
        pressed=blend(normal,p.error,.24f);
        text=p.error;
    }
    if(options.state.readOnly&&!options.state.invalid){
        normal=p.base200;hover=pressed=normal;text=p.baseContent;
        border=options.state.selected?base:p.base300;
    }
    // Preserve semantic contrast for native button content (hover and
    // pressed feedback are conveyed by the background, not flickering text).
    return {normal,hover,pressed,text,border,
            outline||options.state.selected||options.state.invalid||options.state.readOnly};
}
using detail::focus_ring;
using detail::label;
using detail::hint;
using detail::field_begin;
using detail::field_end;

}
ButtonVisual button_visual(Options options){return paint(options);}
float control_height(Size size){
    const float desktop[]={28.f,33.f,38.f,44.f,50.f};
    const float mobile[]={44.f,44.f,48.f,52.f,60.f};
    int i=std::clamp((int)size,0,4);
    return (is_touch()?mobile[i]:desktop[i])*scale();
}
namespace { ControlInfo controlInfo{}; }
namespace detail {
void capture_control(){
    const bool touch=ImGui::GetIO().MouseSource==ImGuiMouseSource_TouchScreen;
    bool focused=ImGui::IsItemFocused();
    if(!focused&&GImGui->NavId&&GImGui->NavIdIsAlive&&GImGui->NavWindow==ImGui::GetCurrentWindow()){
        auto nav=GImGui->NavWindow->NavRectRel[GImGui->NavLayer];nav.Translate(GImGui->NavWindow->Pos);
        focused=GImGui->LastItemData.Rect.Contains(nav);
    }
    // InputScalar with step buttons, ColorEdit and other composite widgets
    // finish in an ImGui item group. The group bounds are real, but the group's
    // LastItemData may not carry the hover of its focused/hovered child.
    // Only report a composite hover when ImGui itself reports a hovered item
    // AND the pointer is within this control's actual submitted rectangle.
    const auto minimum=ImGui::GetItemRectMin(),maximum=ImGui::GetItemRectMax();
    const bool childHovered=!touch&&!ImGui::IsItemHovered()&&
        ImGui::IsAnyItemHovered()&&ImRect(minimum,maximum).Contains(ImGui::GetIO().MousePos);
    controlInfo={{!touch&&(ImGui::IsItemHovered()||childHovered),
        ImGui::IsItemActive(),focused,
        ImGui::IsItemActivated(),bool(GImGui->LastItemData.ItemFlags&ImGuiItemFlags_Disabled),ImGui::IsItemDeactivated()},
        minimum,maximum,GImGui->LastItemData.ID};
}
}
ControlInfo last_control(){return controlInfo;}
Interaction last_interaction(bool disabled){auto result=controlInfo.interaction;result.disabled|=disabled;return result;}
FieldVisual field_visual(Field f){
    const auto& p=palette();
    Color accent=p.primary,border=p.base300;
    const auto feedback=f.invalid?ValidationState::Error:f.validation;
    if(feedback==ValidationState::Error)border=accent=p.error;
    if(feedback==ValidationState::Warning)border=accent=p.warning;
    if(feedback==ValidationState::Success)border=accent=p.success;
    Color normal=f.readOnly?blend(p.base100,p.base300,.22f):p.base100;
    if(feedback!=ValidationState::None)normal=blend(normal,accent,.035f);
    return {normal,blend(normal,accent,.10f),blend(normal,accent,.19f),border,accent};
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
    const float previous=ImGui::GetCurrentWindow()->FontWindowScale;
    ImGui::SetWindowFontScale(previous*1.23f);
    ImGui::TextUnformatted(value?value:"");
    ImGui::SetWindowFontScale(previous);
    ImGui::PopStyleColor();
}
void Muted(const char* value){
    ImGui::PushStyleColor(ImGuiCol_Text,vec(blend(palette().baseContent,palette().base100,.39f)));
    ImGui::TextWrapped("%s",value?value:"");
    ImGui::PopStyleColor();
}
void Divider(){ImGui::Separator();}
void Badge(const char* value,Status status){
    value=value?value:"";const Color c=status_color(status);
    const float pad=8.f*scale(),available=std::max(1.f,ImGui::GetContentRegionAvail().x);
    const float wrap=std::max(1.f,available-pad*2);
    const auto measured=ImGui::CalcTextSize(value,nullptr,false,wrap);
    const ImVec2 size{std::min(available,measured.x+pad*2),measured.y+pad};
    const auto p=ImGui::GetCursorScreenPos();ImGui::Dummy(size);auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(p,{p.x+size.x,p.y+size.y},detail::ink(blend(palette().base200,c,.15f)),std::min(palette().metrics.radiusSelector*scale(),size.y*.5f));
    d->AddText(nullptr,ImGui::GetFontSize(),{p.x+pad,p.y+pad*.5f},detail::ink(c),value,nullptr,wrap);
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
    ImGui::TextWrapped("%s",title?title:"");
    ImGui::PopStyleColor();
    ImGui::TextWrapped("%s",message);
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopID();
}
bool Button(const char* title,Options opt,ImVec2 size){
    title=title?title:"";
    const bool locked=opt.state.disabled||opt.state.loading||opt.state.readOnly;
    const Paint p=paint(opt);const float h=control_height(opt.size);
    const float busy=opt.state.loading?24.f*scale():0.f;
    const auto measured=ImGui::CalcTextSize(title,nullptr,true);
    if(size.x<=0)size.x=std::min(std::max(1.f,ImGui::GetContentRegionAvail().x),measured.x+20.f*scale()+busy);
    if(opt.fullWidth)size.x=std::max(1.f,ImGui::GetContentRegionAvail().x);
    size.y=std::max(size.y,h);
    ImGui::PushStyleColor(ImGuiCol_Button,vec(p.normal));
    const bool touch=ImGui::GetIO().MouseSource==ImGuiMouseSource_TouchScreen;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,vec(touch?p.normal:p.hover));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,vec(p.pressed));
    ImGui::PushStyleColor(ImGuiCol_Text,{p.text.r,p.text.g,p.text.b,0});
    ImGui::PushStyleColor(ImGuiCol_Border,vec(p.border));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,p.outline?palette().metrics.border:0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha,locked&&!opt.state.disabled?1.f:ImGui::GetStyle().DisabledAlpha);
    ImGui::BeginDisabled(locked);
    const bool clicked=ImGui::Button(title,size)&&!locked;
    ImGui::PopStyleColor(); // border
    ImGui::PopStyleColor(); // transparent label
    ImGui::PushStyleColor(ImGuiCol_Text,vec(p.text));
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(a,b,true);
    const float right=std::max(a.x,b.x-busy-8.f*scale());
    if(measured.x>right-a.x-8.f*scale())
        ImGui::RenderTextEllipsis(draw,{a.x+8.f*scale(),(a.y+b.y-measured.y)*.5f},{right,b.y},right,title,ImGui::FindRenderedTextEnd(title),&measured);
    else ImGui::RenderTextClipped({a.x+8.f*scale(),a.y},{right,b.y},title,nullptr,nullptr,{.5f,.5f});
    if(opt.state.loading){
        auto* d=ImGui::GetWindowDrawList();const float start=static_cast<float>(ImGui::GetTime()*3.8f);
        d->PathArcTo({b.x-12.f*scale(),(a.y+b.y)*.5f},5.5f*scale(),start,start+4.2f,16);
        d->PathStroke(ImGui::GetColorU32(vec(p.text)),0,1.8f*scale());
    }
    draw->PopClipRect();focus_ring();detail::capture_control();
    if(measured.x>right-a.x-8.f*scale()&&(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort|ImGuiHoveredFlags_AllowWhenDisabled)||ImGui::IsItemFocused()))
        detail::tooltip_text(title,ImGui::FindRenderedTextEnd(title));
    ImGui::PopStyleColor();ImGui::EndDisabled();ImGui::PopStyleVar(2);ImGui::PopStyleColor(3);
    return clicked;
}
bool Button(const char* label,Variant variant,ImVec2 size,bool enabled){
    Options o;o.variant=variant;o.state.disabled=!enabled;return Button(label,o,size);
}
bool Tabs(const char* id,const char* const* labels,int count,int* selected,Size size){
    if(!labels||!selected||count<=0)return false;
    const float gap=ImGui::GetStyle().ItemSpacing.x;
    const float w=ImGui::GetContentRegionAvail().x;
    float widest=0;
    for(int i=0;i<count;++i)widest=std::max(widest,ImGui::CalcTextSize(labels[i],nullptr,true).x+20.f*scale());
    const int columns=std::max(1,std::min(count,static_cast<int>((w+gap)/(std::max(60.f*scale(),widest)+gap))));
    const float per=std::max(1.f,(w-gap*(columns-1))/columns);
    bool changed=false;
    ImGui::PushID(id);
    for(int i=0;i<count;i++){
        ImGui::PushID(i);
        if(i%columns)ImGui::SameLine();
        Options opt;opt.size=size;opt.variant=*selected==i?Variant::Soft:Variant::Ghost;
        opt.state.selected=*selected==i;
        if(Button(labels[i],opt,{per,0})&&*selected!=i){*selected=i;changed=true;}
        ImGui::PopID();
    }
    ImGui::PopID();
    return changed;
}
ImVec2 lastInputMin{},lastInputMax{};
namespace { InputObserver inputObserver=nullptr;void* inputObserverUser=nullptr;
void input_bounds(Field f){
    lastInputMin=ImGui::GetItemRectMin();lastInputMax=ImGui::GetItemRectMax();
    if(inputObserver&&!f.disabled&&!f.readOnly&&!f.loading&&!(GImGui->CurrentItemFlags&ImGuiItemFlags_Disabled)&&ImGui::IsItemVisible()){
        const auto* window=ImGui::GetCurrentWindow();
        const ImVec2 a{std::max(lastInputMin.x,window->ClipRect.Min.x),std::max(lastInputMin.y,window->ClipRect.Min.y)};
        const ImVec2 b{std::min(lastInputMax.x,window->ClipRect.Max.x),std::min(lastInputMax.y,window->ClipRect.Max.y)};
        inputObserver(a,b,ImGui::IsItemActive(),inputObserverUser);
    }
}
}
void SetInputObserver(InputObserver callback,void* user){inputObserver=callback;inputObserverUser=user;}

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
    input_bounds(field);
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
    input_bounds(field);
    field_end(field);ImGui::PopID();return changed;
}
bool Select(const char* name,const char* const* choices,int count,int* selection,Field field){
    if(!selection||count<0||(!choices&&count>0))return false;
    ImGui::PushID(name);label(name,field);
    field_begin(field,true);
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool valid=*selection>=0&&*selection<count;
    bool changed=false;
    if(ImGui::BeginCombo("##select",valid&&choices[*selection]?choices[*selection]:(count?"Auswählen …":"Keine Optionen"))) {
        if(count==0)ImGui::TextDisabled("Keine Optionen verfügbar.");
        ImGuiListClipper clipper;
        clipper.Begin(count);
        if(valid)clipper.IncludeItemByIndex(*selection);
        while(clipper.Step())for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;++i){
            ImGui::PushID(i);
            const bool chosen=i==*selection;
            if(ImGui::Selectable(choices[i]?choices[i]:"",chosen)&&!field.readOnly){
                *selection=i;changed=true;
            }
            if(chosen)ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    field_end(field);
    ImGui::PopID();
    return changed;
}
namespace {
bool selection_control(const char* name,bool selected,Field field,bool radio){
    name=name?name:"";ImGui::PushID(name);field_begin(field,true);
    const float icons[]={14,18,22,26,30};const float icon=icons[std::clamp(static_cast<int>(field.size),0,4)]*scale(),gap=8.f*scale();
    const float available=std::max(icon,ImGui::GetContentRegionAvail().x);
    const char* end=std::strstr(name,"##");const float wrap=std::max(1.f,available-icon-gap-(field.required?10.f*scale():0));
    const auto measure=ImGui::CalcTextSize(name,end,false,wrap);
    const float h=std::max(control_height(field.size),measure.y),width=std::min(available,icon+gap+measure.x+(field.required?10.f*scale():0));
    const bool clicked=ImGui::InvisibleButton("##choice",{width,h},ImGuiButtonFlags_EnableNav);
    const auto a=ImGui::GetItemRectMin();const ImVec2 p{a.x,a.y+(h-icon)*.5f};
    const auto visual=field_visual(field);const bool filled=selected||(!radio&&field.indeterminate);
    Color bg=filled?visual.accent:visual.normal;
    const bool hover=ImGui::GetIO().MouseSource!=ImGuiMouseSource_TouchScreen&&ImGui::IsItemHovered();
    if(hover)bg=blend(bg,palette().baseContent,.10f);
    if(ImGui::IsItemActive())bg=blend(bg,palette().baseContent,.20f);
    auto* d=ImGui::GetWindowDrawList();
    if(radio){
        const ImVec2 center{p.x+icon*.5f,p.y+icon*.5f};
        d->AddCircleFilled(center,icon*.5f,detail::ink(visual.normal),24);
        d->AddCircle(center,icon*.5f,detail::ink(filled?bg:visual.border),24,1.5f*scale());
        if(filled)d->AddCircleFilled(center,icon*.25f,detail::ink(bg),20);
    }else{
        const ImVec2 b{p.x+icon,p.y+icon};const float radius=std::min(icon*.5f,palette().metrics.radiusSelector*scale());
        d->AddRectFilled(p,b,detail::ink(bg),radius);
        d->AddRect(p,b,detail::ink(filled?visual.accent:visual.border),radius,0,palette().metrics.border*scale());
        const auto content=detail::ink(field.invalid||field.validation!=ValidationState::None?palette().base100:palette().primaryContent);
        if(field.indeterminate)d->AddLine({p.x+icon*.25f,p.y+icon*.5f},{p.x+icon*.75f,p.y+icon*.5f},content,2.f*scale());
        else if(selected){d->AddLine({p.x+icon*.22f,p.y+icon*.50f},{p.x+icon*.43f,p.y+icon*.72f},content,2.f*scale());
            d->AddLine({p.x+icon*.43f,p.y+icon*.72f},{p.x+icon*.80f,p.y+icon*.26f},content,2.f*scale());}
    }
    const float textY=a.y+(h-measure.y)*.5f;
    d->AddText(nullptr,ImGui::GetFontSize(),{a.x+icon+gap,textY},ImGui::GetColorU32(ImGuiCol_Text),name,end,wrap);
    if(field.required)d->AddText({a.x+icon+gap+measure.x+3.f*scale(),textY},detail::ink(palette().error),"*");
    field_end(field);ImGui::PopID();return clicked;
}
}
bool Checkbox(const char* name,bool* value,Field field){
    if(!value)return false;
    const bool changed=selection_control(name,*value,field,false);
    if(changed)*value=field.indeterminate?true:!*value;
    return changed;
}
bool Radio(const char* name,int item,int* selected,Field field){
    if(!selected)return false;
    const bool changed=selection_control(name,*selected==item,field,true)&&*selected!=item;
    if(changed)*selected=item;
    return changed;
}
bool Switch(const char* name,bool* value,Field field){
    if(!value)return false;
    name=name?name:"";ImGui::PushID(name);field_begin(field,true);
    const float w=48.f*scale(),gap=8.f*scale();
    const float available=std::max(w,ImGui::GetContentRegionAvail().x);
    const char* end=std::strstr(name,"##");
    const float wrap=std::max(1.f,available-w-gap);
    const auto measure=ImGui::CalcTextSize(name,end,false,wrap);
    const float h=std::max(control_height(field.size),measure.y);
    const float width=std::min(available,w+(measure.x>0?gap+measure.x:0));
    const bool clicked=ImGui::InvisibleButton("##switch",{width,h},ImGuiButtonFlags_EnableNav);
    if(clicked)*value=!*value;
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    const float trackH=std::min(h,23.f*scale()),r=trackH*.5f,cy=(a.y+b.y)*.5f;
    const auto visual=field_visual(field);
    Color bg=*value?visual.accent:palette().base300;
    if(field.invalid||field.validation!=ValidationState::None)bg=visual.accent;
    const bool hover=ImGui::GetIO().MouseSource!=ImGuiMouseSource_TouchScreen&&ImGui::IsItemHovered();
    if(hover)bg=blend(bg,palette().baseContent,.12f);
    if(ImGui::IsItemActive())bg=blend(bg,palette().baseContent,.22f);
    auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled({a.x,cy-r},{a.x+w,cy+r},detail::ink(bg),r);
    d->AddCircleFilled({a.x+(*value?w-r:r),cy},r-3.f*scale(),detail::ink(*value?palette().primaryContent:palette().baseContent),24);
    d->AddText(nullptr,ImGui::GetFontSize(),{a.x+w+gap,a.y+(h-measure.y)*.5f},ImGui::GetColorU32(ImGuiCol_Text),name,end,wrap);
    detail::inline_required(field);field_end(field);ImGui::PopID();return clicked;
}
bool Slider(const char* name,float* value,float min,float max,const char* fmt,Field field){
    if(!value)return false;
    ImGui::PushID(name);label(name,field);
    field_begin(field,true);
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool changed=ImGui::SliderFloat("##slider",value,min,max,fmt,
                    field.readOnly?ImGuiSliderFlags_NoInput:0) && !field.readOnly;
    ui::scroll::set_drag_control_active(ImGui::IsItemActive());
    field_end(field);ImGui::PopID();return changed;
}
void Progress(const char* title,float fraction,Status status){
    ImGui::PushID(title);
    ImGui::TextUnformatted(title);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,vec(status_color(status)));
    ImGui::ProgressBar(std::isfinite(fraction)?std::clamp(fraction,0.f,1.f):0.f,{-FLT_MIN,11.f*scale()},"");
    ImGui::PopStyleColor();
    ImGui::PopID();
}
void Tooltip(const char* info){
    if(info&&(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort|ImGuiHoveredFlags_AllowWhenDisabled)||ImGui::IsItemFocused()))
        detail::tooltip_text(info);
}
void OpenModal(const char* id){ImGui::OpenPopup(id);}
bool BeginModal(const char* id,bool* open){
    ImGui::SetNextWindowSizeConstraints({280.f,0.f},{640.f,FLT_MAX});
    return ImGui::BeginPopupModal(id,open,ImGuiWindowFlags_AlwaysAutoResize);
}
void EndModal(){ImGui::EndPopup();}
bool BeginCard(const char* id,float height){
    ImGui::PushStyleColor(ImGuiCol_ChildBg,vec(palette().base100));
    ImGui::PushStyleColor(ImGuiCol_Border,vec(palette().base300));
    ImGuiChildFlags flags=ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened;
    if(height<=0.f)flags|=ImGuiChildFlags_AutoResizeY;
    return ImGui::BeginChild(id,{0,height<=0?0:height},flags);
}
void EndCard(){ImGui::EndChild();ImGui::PopStyleColor(2);}
}
