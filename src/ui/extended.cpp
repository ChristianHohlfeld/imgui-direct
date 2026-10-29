#include "ui/extended.hpp"
#include "ui/scroll.hpp"
#include "ui/field_detail.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <limits>
#include <cstring>

namespace ui {
namespace {
ImVec4 v(Color c){ return {c.r,c.g,c.b,c.a}; }
ImU32 ink(Color c){ return ImGui::ColorConvertFloat4ToU32(v(c)); }
const char* text(const char* s){ return s?s:""; }
Color statusColor(Status s){
    switch(s){case Status::Success:return palette().success;case Status::Warning:return palette().warning;
    case Status::Error:return palette().error;default:return palette().info;}
}
float unit(float f){return std::isfinite(f)?std::clamp(f,0.f,1.f):0.f;}
struct EditScope:detail::FieldScope {
    using detail::FieldScope::FieldScope;
    ~EditScope(){scroll::set_drag_control_active(ImGui::IsItemActive());}
};

}
int page_count(int total,int size){ return total>0&&size>0?1+(total-1)/size:0; }
bool Navigation(const char* id,std::span<const NavItem> items,int* selected,bool vertical,Size size){
    if(!selected||items.empty())return false;
    ImGui::PushID(id);bool changed=false;
    for(std::size_t i=0;i<items.size();++i){
        if(i&&!vertical)ImGui::SameLine();
        ImGui::PushID(static_cast<int>(i));
        Options o;o.size=size;o.variant=Variant::Ghost;o.fullWidth=vertical;
        o.state.selected=*selected==static_cast<int>(i);o.state.disabled=items[i].disabled;
        if(Button(text(items[i].label),o)&&!o.state.selected){*selected=static_cast<int>(i);changed=true;}
        ImGui::PopID();
    }
    ImGui::PopID();return changed;
}
bool Breadcrumbs(const char* id,std::span<const char* const> path,int* activated){
    if(!activated||path.empty())return false;
    bool clicked=false;ImGui::PushID(id);
    // Vertical on narrow panels: retain every ancestor rather than hide navigation.
    const bool narrow=ImGui::GetContentRegionAvail().x<320.f*scale();
    for(std::size_t i=0;i<path.size();++i){
        if(i){if(!narrow)ImGui::SameLine();ImGui::TextUnformatted("/");if(!narrow)ImGui::SameLine();}
        ImGui::PushID(static_cast<int>(i));Options o;o.variant=Variant::Ghost;o.size=Size::SM;
        o.state.disabled=i+1==path.size();
        if(Button(text(path[i]),o)){*activated=static_cast<int>(i);clicked=true;}
        ImGui::PopID();
    }
    ImGui::PopID();return clicked;
}
bool Pagination(const char* id,int total,int size,int* page){
    if(!page)return false;
    const int count=page_count(total,size),before=*page;
    *page=std::clamp(*page,0,std::max(0,count-1));
    ImGui::PushID(id);Options o;o.size=Size::SM;o.variant=Variant::Ghost;
    o.state.disabled=*page==0||count==0;
    if(Button("<",o))--*page;
    ImGui::SameLine();ImGui::Text("%d / %d",count?*page+1:0,count);ImGui::SameLine();
    o.state.disabled=count==0||*page>=count-1;
    if(Button(">",o))++*page;
    ImGui::PopID();return before!=*page;
}
bool Collapse(const char* label,bool* open,bool disabled){
    if(!open)return false;
    ImGui::BeginDisabled(disabled);
    ImGui::SetNextItemOpen(*open,ImGuiCond_Always);
    *open=ImGui::CollapsingHeader(label);
    detail::capture_control();
    ImGui::EndDisabled();return *open;
}
bool Accordion(const char* label,int index,int* openIndex,bool disabled){
    if(!openIndex||index<0)return false;
    bool open=*openIndex==index;const bool before=open;
    Collapse(label,&open,disabled);
    if(open!=before&&!disabled)*openIndex=open?index:-1;
    return *openIndex==index;
}
bool BeginDropdown(const char* label,Options options){
    if(Button(label,options))ImGui::OpenPopup(label);
    return ImGui::BeginPopup(label);
}
void EndDropdown(){ImGui::EndPopup();}
bool BeginContextMenu(const char* id){return ImGui::BeginPopupContextItem(id);}
void EndContextMenu(){ImGui::EndPopup();}
bool MenuItem(const char* label,const char* shortcut,bool selected,bool enabled){
    const bool clicked=ImGui::MenuItem(label,shortcut,selected,enabled);
    detail::capture_control();return clicked;
}
void BeginFieldset(const char* id,const char* legend,bool disabled){
    ImGui::PushID(id);ImGui::BeginGroup();ImGui::BeginDisabled(disabled);
    if(legend&&*legend)ImGui::SeparatorText(legend);
}
void EndFieldset(){ImGui::EndDisabled();ImGui::EndGroup();ImGui::PopID();}
void Label(const char* label,bool required){
    const char* shown=text(label);
    ImGui::PushTextWrapPos();ImGui::TextUnformatted(shown,std::strstr(shown,"##"));ImGui::PopTextWrapPos();
    if(required){ImGui::SameLine();ImGui::TextColored(v(palette().error),"*");}
}
void Validation(const char* message,bool valid){
    ImGui::PushStyleColor(ImGuiCol_Text,v(valid?blend(palette().baseContent,palette().base100,.4f):palette().error));
    ImGui::TextWrapped("%s",text(message));ImGui::PopStyleColor();
}
void Validation(const char* message,ValidationState state){
    Field f;f.validation=state;const Color color=state==ValidationState::None?blend(palette().baseContent,palette().base100,.4f):field_visual(f).border;
    ImGui::PushStyleColor(ImGuiCol_Text,v(color));ImGui::TextWrapped("%s",text(message));ImGui::PopStyleColor();
}
void Divider(const char* label){ImGui::SeparatorText(text(label));}
void Kbd(const char* shortcut){
    const auto p=ImGui::GetCursorScreenPos(),s=ImGui::CalcTextSize(text(shortcut));
    const float pad=5.f*scale();
    ImGui::Dummy({s.x+pad*2,s.y+pad*2});
    auto* d=ImGui::GetWindowDrawList();const auto b=ImGui::GetItemRectMax();
    d->AddRectFilled(p,b,ink(palette().base200),4.f*scale());
    d->AddRect(p,b,ink(palette().base300),4.f*scale());
    d->AddText({p.x+pad,p.y+pad},ink(palette().baseContent),text(shortcut));
}
void StatusDot(Status status,const char* label){
    const float r=4.f*scale();const auto p=ImGui::GetCursorScreenPos();
    ImGui::Dummy({r*2,ImGui::GetTextLineHeight()});
    ImGui::GetWindowDrawList()->AddCircleFilled({p.x+r,p.y+ImGui::GetTextLineHeight()*.5f},r,ink(statusColor(status)));
    if(label){ImGui::SameLine();ImGui::TextUnformatted(label);}
}
void Avatar(const char* initials,Size size){
    const float h=control_height(size);const auto p=ImGui::GetCursorScreenPos();
    ImGui::Dummy({h,h});auto* d=ImGui::GetWindowDrawList();
    d->AddCircleFilled({p.x+h*.5f,p.y+h*.5f},h*.5f,ink(palette().secondary));
    const auto s=ImGui::CalcTextSize(text(initials));
    // Clip long initials to the avatar box. Caller may use an image via ImGui::Image instead.
    d->PushClipRect(p,{p.x+h,p.y+h},true);
    d->AddText({p.x+(h-s.x)*.5f,p.y+(h-s.y)*.5f},ink(palette().secondaryContent),text(initials));
    d->PopClipRect();
}
void Stat(const char* label,const char* value,const char* description){
    ImGui::BeginGroup();Muted(text(label));Heading(text(value));
    if(description)Muted(description);
    ImGui::EndGroup();
}
void Steps(const char* id,std::span<const char* const> labels,int current){
    ImGui::PushID(id);const float diameter=24.f*scale(),gap=8.f*scale();
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x-diameter-gap);
    for(std::size_t i=0;i<labels.size();++i){
        const auto p=ImGui::GetCursorScreenPos();const auto measured=ImGui::CalcTextSize(text(labels[i]),nullptr,false,width);
        const float h=std::max(diameter,measured.y);const ImVec2 center{p.x+diameter*.5f,p.y+diameter*.5f};
        const bool complete=static_cast<int>(i)<current,active=static_cast<int>(i)==current;
        auto* d=ImGui::GetWindowDrawList();const Color color=complete?palette().success:active?palette().primary:palette().base300;
        if(i+1<labels.size())d->AddLine({center.x,center.y+diameter*.5f},{center.x,p.y+h+ImGui::GetStyle().ItemSpacing.y+diameter*.5f},detail::ink(color),2.f*scale());
        if(complete||active)d->AddCircleFilled(center,diameter*.5f,detail::ink(color),24);
        else d->AddCircle(center,diameter*.5f,detail::ink(color),24,1.5f*scale());
        if(complete){
            const auto c=detail::ink(palette().base100);
            d->AddLine({center.x-5.f*scale(),center.y},{center.x-1.f*scale(),center.y+4.f*scale()},c,2.f*scale());
            d->AddLine({center.x-1.f*scale(),center.y+4.f*scale()},{center.x+6.f*scale(),center.y-4.f*scale()},c,2.f*scale());
        }else{
            char number[16];ImFormatString(number,sizeof(number),"%d",static_cast<int>(i)+1);const auto s=ImGui::CalcTextSize(number);
            d->AddText({center.x-s.x*.5f,center.y-s.y*.5f},detail::ink(active?palette().primaryContent:palette().baseContent),number);
        }
        d->AddText(nullptr,ImGui::GetFontSize(),{p.x+diameter+gap,p.y+(h-measured.y)*.5f},ImGui::GetColorU32(ImGuiCol_Text),text(labels[i]),nullptr,width);
        ImGui::Dummy({diameter+gap+std::min(width,measured.x),h});
    }
    ImGui::PopID();
}
void TimelineItem(const char* title,const char* description,Status status){
    ImGui::BeginGroup();StatusDot(status,title);if(description)Muted(description);
    ImGui::EndGroup();
}
void Loading(const char* id,Size size){
    ImGui::PushID(id);const float h=control_height(size),r=h*.27f;
    const auto p=ImGui::GetCursorScreenPos();ImGui::Dummy({h,h});
    auto* d=ImGui::GetWindowDrawList();const float a=static_cast<float>(ImGui::GetTime()*4.0);
    d->PathArcTo({p.x+h*.5f,p.y+h*.5f},r,a,a+4.5f,20);
    d->PathStroke(ink(palette().primary),0,2.f*scale());ImGui::PopID();
}
void RadialProgress(const char* id,float fraction,Size size,Status status){
    ImGui::PushID(id);const float h=control_height(size)*1.5f,r=h*.42f;
    const auto p=ImGui::GetCursorScreenPos();ImGui::Dummy({h,h});
    auto* d=ImGui::GetWindowDrawList();const ImVec2 center{p.x+h*.5f,p.y+h*.5f};
    d->AddCircle(center,r,ink(palette().base300),32,3.f*scale());
    const float f=unit(fraction);
    if(f>0){d->PathArcTo(center,r,-1.5707963f,-1.5707963f+6.2831853f*f,32);d->PathStroke(ink(statusColor(status)),0,3.f*scale());}
    char number[8];ImFormatString(number,sizeof(number),"%.0f%%",f*100.f);const auto s=ImGui::CalcTextSize(number);
    d->AddText({center.x-s.x*.5f,center.y-s.y*.5f},ink(palette().baseContent),number);ImGui::PopID();
}
void Skeleton(const char* id,ImVec2 size){
    ImGui::PushID(id);size.x=size.x<=0?std::max(1.f,ImGui::GetContentRegionAvail().x):size.x;
    size.y=std::max(1.f,size.y);const auto p=ImGui::GetCursorScreenPos();ImGui::Dummy(size);
    // Static by default: no decorative animation or idle redraw requirement.
    ImGui::GetWindowDrawList()->AddRectFilled(p,{p.x+size.x,p.y+size.y},ink(palette().base300),6.f*scale());ImGui::PopID();
}
bool BeginTable(const char* id,std::span<const TableColumn> columns,TableOptions options){
    if(columns.empty()||columns.size()>64)return false;
    auto flags=options.flags;
    if(options.sortable)flags|=ImGuiTableFlags_Sortable;
    if(options.height>0)flags|=ImGuiTableFlags_ScrollY;
    const auto pending=GImGui->NextWindowData;
    if(flags&(ImGuiTableFlags_ScrollY|ImGuiTableFlags_ScrollX))ImGui::SetNextWindowChildFlags(ImGuiChildFlags_NavFlattened,true);
    if(!ImGui::BeginTable(id,static_cast<int>(columns.size()),flags,{0,options.height})){
        GImGui->NextWindowData=pending;return false;
    }
    for(std::size_t i=0;i<columns.size();++i){const auto& c=columns[i];
        ImGui::TableSetupColumn(text(c.label),c.flags|(c.width>0?ImGuiTableColumnFlags_WidthFixed:0),c.width,static_cast<ImGuiID>(i));}
    ImGui::TableSetupScrollFreeze(0,1);ImGui::TableHeadersRow();return true;
}
void EndTable(){ImGui::EndTable();}
int VirtualList(const char* id,int count,float height,float rowHeight,RowRenderer render,void* user){
    if(count<=0||!render||!std::isfinite(rowHeight)||rowHeight<=0||!std::isfinite(height)||height<=0)return 0;
    int submitted=0;
    if(ImGui::BeginChild(id,{0,height},ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened)){
        ImGuiListClipper clip;clip.Begin(count,rowHeight);
        while(clip.Step())for(int row=clip.DisplayStart;row<clip.DisplayEnd;++row){
            ImGui::PushID(row);render(row,user);ImGui::PopID();++submitted;
        }
        scroll::apply();
    }
    ImGui::EndChild();return submitted;
}
bool TreeNode(const char* id,const char* label,bool selected,bool leaf){
    auto flags=ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_OpenOnDoubleClick|ImGuiTreeNodeFlags_SpanAvailWidth;
    if(selected)flags|=ImGuiTreeNodeFlags_Selected;
    if(leaf)flags|=ImGuiTreeNodeFlags_Leaf;
    const bool expanded=ImGui::TreeNodeEx(id,flags,"%s",text(label));
    detail::capture_control();return expanded;
}
void EndTreeNode(){ImGui::TreePop();}
bool BeginSplit(const char* id,float* fraction,ImVec2 size,bool vertical,float minPane){
    // Layout metadata is on the ImGui window storage under this split's ID.
    // It is per-context and doesn't own the actual split ratio.
    ImGui::PushID(id);ImGui::BeginGroup();
    auto* store=ImGui::GetStateStorage();const auto key=ImGui::GetID("##split_axis");
    store->SetBool(key,vertical);
    const auto available=ImGui::GetContentRegionAvail();
    if(size.x<=0)size.x=available.x;
    if(size.y<=0)size.y=available.y;
    const float bar=std::max(6.f*scale(),is_touch()?16.f*scale():0.f);
    const float extent=std::max(2.f,vertical?size.y:size.x),usable=std::max(2.f,extent-bar);
    const float lower=std::min(std::max(1.f,minPane),usable*.5f),limit=lower/usable;
    float ratio=fraction?unit(*fraction):.5f;ratio=std::clamp(ratio,limit,1.f-limit);
    ImVec2 first=size; if(vertical)first.y=usable*ratio;else first.x=usable*ratio;
    // Store geometry before entering child: NextSplit sees parent storage again.
    store->SetFloat(ImGui::GetID("##split_x"),size.x);
    store->SetFloat(ImGui::GetID("##split_y"),size.y);
    store->SetFloat(ImGui::GetID("##split_first"),usable*ratio);
    store->SetFloat(ImGui::GetID("##split_bar"),bar);
    store->SetFloat(ImGui::GetID("##split_lower"),limit);
    store->SetVoidPtr(ImGui::GetID("##split_ratio"),fraction);
    if(fraction)*fraction=ratio;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});
    return ImGui::BeginChild("##first",first,ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened);
}
bool NextSplit(){
    ImGui::EndChild();auto* s=ImGui::GetStateStorage();
    const bool vertical=s->GetBool(ImGui::GetID("##split_axis"));
    const float x=s->GetFloat(ImGui::GetID("##split_x")),y=s->GetFloat(ImGui::GetID("##split_y"));
    const float first=s->GetFloat(ImGui::GetID("##split_first")),bar=s->GetFloat(ImGui::GetID("##split_bar"));
    const float lower=s->GetFloat(ImGui::GetID("##split_lower"));
    auto* ratio=static_cast<float*>(s->GetVoidPtr(ImGui::GetID("##split_ratio")));
    if(!vertical)ImGui::SameLine(0,0);
    const bool pressed=ImGui::InvisibleButton("##grip",vertical?ImVec2(x,bar):ImVec2(bar,y),ImGuiButtonFlags_EnableNav);
    (void)pressed;
    if(!(GImGui->CurrentItemFlags&ImGuiItemFlags_Disabled)&&ImGui::IsItemActive()&&ratio){
        const float extent=std::max(2.f,(vertical?y:x)-bar);
        const auto delta=ImGui::GetIO().MouseDelta;
        *ratio=std::clamp(*ratio+(vertical?delta.y:delta.x)/extent,lower,1.f-lower);
        scroll::set_drag_control_active(true);
    }
    if(ImGui::IsItemHovered()||ImGui::IsItemActive()){
        ImGui::SetMouseCursor(vertical?ImGuiMouseCursor_ResizeNS:ImGuiMouseCursor_ResizeEW);
        ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),ink(palette().primary));
    }
    // Arrow keys resize a focused grip, at the same clamp as pointer/touch input.
    if(!(GImGui->CurrentItemFlags&ImGuiItemFlags_Disabled)&&ratio&&ImGui::IsItemFocused()){
        const auto prev=vertical?ImGuiKey_UpArrow:ImGuiKey_LeftArrow,next=vertical?ImGuiKey_DownArrow:ImGuiKey_RightArrow;
        if(ImGui::IsKeyPressed(prev))*ratio=std::clamp(*ratio-.02f,lower,1.f-lower);
        if(ImGui::IsKeyPressed(next))*ratio=std::clamp(*ratio+.02f,lower,1.f-lower);
    }
    detail::capture_control();
    if(!vertical)ImGui::SameLine(0,0);
    return ImGui::BeginChild("##second",vertical?ImVec2(x,std::max(1.f,y-first-bar)):ImVec2(std::max(1.f,x-first-bar),y),ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened);
}
void EndSplit(){
    ImGui::EndChild();
    ImGui::GetStateStorage()->SetVoidPtr(ImGui::GetID("##split_ratio"),nullptr);
    ImGui::PopStyleVar();ImGui::EndGroup();ImGui::PopID();
}
bool Number(const char* label,float* value,float speed,float min,float max,Field field){
    if(!value||!std::isfinite(speed)||speed<=0||!std::isfinite(min)||!std::isfinite(max))return false;
    EditScope scope(label,field);return ImGui::DragFloat("##value",value,speed,min,max,"%.3f",ImGuiSliderFlags_AlwaysClamp);
}
bool Integer(const char* label,int* value,int step,Field field){
    if(!value||step<=0)return false;
    EditScope scope(label,field);return ImGui::InputInt("##value",value,step,step);
}
bool Vector3(const char* label,float value[3],float speed,Field field){
    if(!value||!std::isfinite(speed)||speed<=0)return false;
    EditScope scope(label,field);return ImGui::DragFloat3("##value",value,speed);
}
bool ColorEdit(const char* label,float rgba[4],Field field){
    if(!rgba)return false;
    EditScope scope(label,field);return ImGui::ColorEdit4("##value",rgba,ImGuiColorEditFlags_AlphaBar);
}
bool Range(const char* label,float* low,float* high,float min,float max,Field field){
    if(!low||!high||low==high||!std::isfinite(min)||!std::isfinite(max)||min>=max)return false;
    EditScope scope(label,field);
    // ImGui bounds both handles against each other; repair external invalid data only if editable.
    bool repaired=false;
    if(!(GImGui->CurrentItemFlags&ImGuiItemFlags_Disabled)){
        const float a=std::clamp(std::isfinite(*low)?*low:min,min,max);
        const float b=std::clamp(std::isfinite(*high)?*high:max,min,max);
        const float nextLow=std::min(a,b),nextHigh=std::max(a,b);
        repaired=*low!=nextLow||*high!=nextHigh;*low=nextLow;*high=nextHigh;
    }
    return ImGui::DragFloatRange2("##value",low,high,std::max(.001f,(max-min)*.005f),min,max,"%.3f","%.3f",ImGuiSliderFlags_AlwaysClamp)||repaired;
}
bool FileInput(const char* id,const char* name,FilePicker picker,Field field){
    ImGui::PushID(id);Label(id,field.required);
    Options o;o.size=field.size;o.variant=Variant::Outline;o.state.disabled=field.disabled||!picker.request;
    o.state.readOnly=field.readOnly;o.state.loading=field.loading;o.state.invalid=field.invalid||field.validation==ValidationState::Error;
    if(field.validation==ValidationState::Warning)o.variant=Variant::Warning;
    if(field.validation==ValidationState::Success)o.variant=Variant::Success;
    const bool request=Button("Datei auswählen",o);
    const auto control=GImGui->LastItemData;
    if(name&&*name)ImGui::TextWrapped("%s",name);
    if(field.help)Validation(field.help,field.invalid?ValidationState::Error:field.validation);
    GImGui->LastItemData=control;ImGui::PopID();
    if(request)picker.request(picker.user);
    return request;
}
unsigned ToastQueue::push(const char* message,Status status,double now,double duration){
    if(!message||!*message||!std::isfinite(now)||!std::isfinite(duration)||duration<0)return 0;
    const double expiry=duration>0?now+duration:0;
    if(duration>0&&(!std::isfinite(expiry)||expiry==0))return 0;
    if(count_==capacity)dismiss(items_[0].id);
    // Zero is reserved for failure. Keep handles unique even on counter wrap.
    do{++next_;}while(next_==0||std::any_of(items_.begin(),items_.begin()+count_,[&](const Entry& e){return e.id==next_;}));
    auto& e=items_[count_++];e.message=message;e.status=status;e.expires=expiry;e.id=next_;return e.id;
}
bool ToastQueue::dismiss(unsigned id){
    for(std::size_t i=0;i<count_;++i)if(items_[i].id==id){
        for(std::size_t j=i+1;j<count_;++j)items_[j-1]=std::move(items_[j]);
        items_[--count_]=Entry{};return true;
    }
    return false;
}
void ToastQueue::update(double now){
    if(!std::isfinite(now))return;
    for(std::size_t i=0;i<count_;)if(items_[i].expires!=0&&now>=items_[i].expires)dismiss(items_[i].id);else ++i;
}
void Toasts(const char* id,ToastQueue& queue,double now){
    queue.update(now);if(queue.entries().empty())return;
    const auto* vp=ImGui::GetMainViewport();const float margin=12.f*scale();
    ImGui::SetNextWindowPos({vp->WorkPos.x+vp->WorkSize.x-margin,vp->WorkPos.y+margin},ImGuiCond_Always,{1,0});
    ImGui::SetNextWindowSize({std::min(360.f*scale(),std::max(1.f,vp->WorkSize.x-margin*2)),0});
    unsigned dismissed=0;
    // A toast is a transient notification overlay, not the active dialog.
    // NoFocusOnAppearing alone avoids only the initial focus steal; it still
    // leaves this top-level overlay eligible for nav-window traversal. That
    // can trap Tab outside the underlying form after a toast is emitted.
    // Keep pointer/touch dismissal and the native button, but prevent this
    // passive overlay from replacing the host's keyboard navigation window.
    if(ImGui::Begin(id,nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|
         ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoTitleBar|
         ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNavFocus)){
        for(const auto& entry:queue.entries()){
            ImGui::PushID(static_cast<int>(entry.id));
            StatusDot(entry.status);ImGui::SameLine();
            if(ImGui::SmallButton("x"))dismissed=entry.id;
            ImGui::PushTextWrapPos();ImGui::TextUnformatted(entry.message.c_str());ImGui::PopTextWrapPos();
            ImGui::Separator();ImGui::PopID();
        }
    }
    ImGui::End();if(dismissed)queue.dismiss(dismissed);
}
}
