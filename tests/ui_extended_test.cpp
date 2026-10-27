#include "ui/extended.hpp"
#include "imgui_internal.h" // Tests only: verify IDs/nav and balanced scopes.
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {
void begin(){
    ui::begin_frame();ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({720,900});
    ImGui::Begin("test",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoTitleBar);
}
void end(){ImGui::End();ImGui::Render();}
ImVec2 center(){auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();return {(a.x+b.x)*.5f,(a.y+b.y)*.5f};}
void row(int i,void* user){
    ++*static_cast<int*>(user);char s[32];std::snprintf(s,sizeof(s),"row %d",i);
    ImGui::Selectable(s,false,0,{0,32-ImGui::GetStyle().ItemSpacing.y});
}
}
int main(){
    assert(ui::page_count(INT_MAX,2)==1073741824);
    assert(ui::page_count(11,5)==3&&ui::page_count(0,5)==0&&ui::page_count(10,0)==0);
    ui::ToastQueue queue;
    assert(!queue.push("",ui::Status::Info,0));
    assert(!queue.push("bad",ui::Status::Info,std::numeric_limits<double>::quiet_NaN()));
    auto sticky=queue.push("persistent",ui::Status::Info,0,0);
    queue.push("temporary",ui::Status::Success,0,2);
    queue.update(2);assert(queue.entries().size()==1&&queue.entries()[0].id==sticky);
    for(int i=0;i<12;++i)queue.push("event",ui::Status::Warning,2,0);
    assert(queue.entries().size()==queue.capacity&&!queue.dismiss(sticky));
    assert(queue.dismiss(queue.entries()[3].id)&&queue.entries().size()==7);
    queue.push("full",ui::Status::Info,2,0);
    const auto oldest=queue.entries()[0].id;
    assert(!queue.push("overflow",ui::Status::Info,std::numeric_limits<double>::max(),std::numeric_limits<double>::max()));
    assert(queue.entries().size()==8&&queue.entries()[0].id==oldest);
    IMGUI_CHECKVERSION();ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={720,900};io.DeltaTime=1.f/60;
    ui::initialize();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    int observed=0;
    ui::SetInputObserver([](ImVec2 a,ImVec2 b,bool,void* user){
        assert(b.x>a.x&&b.y>a.y);++*static_cast<int*>(user);
    },&observed);
    begin();char input[32]="text";
    ui::Input("Input##one",input,sizeof(input));
    ui::Textarea("Textarea##two",input,sizeof(input));
    const int editable=observed;
    ui::Field disabled;disabled.disabled=true;ui::Input("Disabled",input,sizeof(input),"",disabled);
    ui::Field readOnly;readOnly.readOnly=true;ui::Input("Readonly",input,sizeof(input),"",readOnly);
    ui::BeginFieldset("parent-disabled","Inherited",true);ui::Input("Inherited",input,sizeof(input));ui::EndFieldset();
    assert(editable==2&&observed==editable);end();ui::SetInputObserver(nullptr);
    bool toggled=false;
    begin();const auto origin=ImGui::GetCursorScreenPos();ui::Switch("switch",&toggled);const auto switchPos=center();end();
    // Use the leading switch track; the complete label is also actionable.
    (void)switchPos;const ImVec2 target{origin.x+24,origin.y+19};
    io.AddMousePosEvent(target.x,target.y);
    begin();ui::Switch("switch",&toggled);end();
    io.AddMouseButtonEvent(0,true);
    begin();ui::Switch("switch",&toggled);end();
    io.AddMouseButtonEvent(0,false);
    begin();ui::Switch("switch",&toggled);end();assert(toggled);
    // Keyboard activation must work for custom controls, not only standard buttons.
    begin();ui::Switch("switch",&toggled);
    const auto switchId=ImGui::GetCurrentWindow()->GetID("switch");
    // Switch uses PushID(label), then ##switch as actual item ID.
    ImGui::PushID("switch");const auto item=ImGui::GetID("##switch");ImGui::PopID();
    (void)switchId;ImGui::SetNavID(item,ImGuiNavLayer_Main,0,ImRect({0,0},{50,50}));ImGui::SetNavCursorVisible(true);end();
    io.AddKeyEvent(ImGuiKey_Space,true);
    begin();ui::Switch("switch",&toggled);end();
    io.AddKeyEvent(ImGuiKey_Space,false);
    begin();ui::Switch("switch",&toggled);end();assert(!toggled);

    // Closed popups must leave exactly the caller's ID/style stacks intact.
    begin();const int stack=ImGui::GetCurrentWindow()->IDStack.Size;
    assert(!ui::BeginDropdown("menu"));assert(ImGui::GetCurrentWindow()->IDStack.Size==stack);
    ImGui::OpenPopup("menu");
    if(ui::BeginDropdown("menu")){ui::MenuItem("Action");ui::EndDropdown();}
    assert(ImGui::GetCurrentWindow()->IDStack.Size==stack);end();
    // Each component under narrow/large scales, both palettes, mouse/touch.
    for(bool touch:{false,true})for(auto mode:{ui::Mode::Dark,ui::Mode::Light}){
        ui::set_touch(touch);ui::set_mode(mode);ui::set_scale(touch?1.5f:1.f);
        begin();ui::BeginFieldset("form","Transform");
        float number=3,vector[3]={1,2,3},color[4]={1,0,0,1},low=9,high=2;int integer=1;
        ui::Number("number",&number);ui::Integer("integer",&integer);ui::Vector3("position",vector);ui::ColorEdit("color",color);
        assert(ui::Range("range",&low,&high,0,10)&&low==2&&high==9);
        ui::Field locked;locked.readOnly=true;low=8;high=1;
        assert(!ui::Range("readonly",&low,&high,0,10,locked)&&low==8&&high==1);
        assert(!ui::FileInput("file","asset.mesh",{}));
        ui::EndFieldset();
        ui::BeginFieldset("disabled","Disabled",true);
        low=9;high=1;assert(!ui::Range("inherited readonly",&low,&high,0,10)&&low==9&&high==1);
        ui::EndFieldset();end();
        begin();
        const ui::NavItem items[]={{"Scene"},{"Assets",true},{"Play"}};int selected=0;
        ui::Navigation("nav",items,&selected);const char* path[]={"Project","Scenes","Main"};int activated=-1;
        ui::Breadcrumbs("path",path,&activated);int page=99;assert(ui::Pagination("pages",11,5,&page)&&page==2);
        ui::Kbd("Ctrl+S");ui::Avatar("CH");ui::StatusDot(ui::Status::Success,"Ready");ui::Stat("FPS","60");
        ui::Steps("steps",path,1);ui::TimelineItem("Loaded","Main scene");ui::Loading("busy");
        ui::RadialProgress("radial",std::numeric_limits<float>::quiet_NaN());ui::Skeleton("skeleton",{0,20});
        int open=1;assert(ui::Accordion("closed",0,&open)==false);assert(ui::Accordion("open",1,&open));
        end();
        begin();const ui::TableColumn columns[]={{"Actor"},{"Type"}};ui::TableOptions opts;opts.height=120;opts.sortable=true;
        if(ui::BeginTable("actors",columns,opts)){
            ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::TextUnformatted("Camera");
            ImGui::TableNextColumn();ImGui::TextUnformatted("Perspective");ui::EndTable();
        }
        if(ui::TreeNode("root","Scene")){if(ui::TreeNode("leaf","Camera",true,true))ui::EndTreeNode();ui::EndTreeNode();}
        float fraction=.3f;
        const bool first=ui::BeginSplit("split",&fraction,{600,120});if(first)ImGui::TextUnformatted("Outliner");
        if(ui::NextSplit())ImGui::TextUnformatted("Inspector");ui::EndSplit();
        bool visible=ui::BeginSplit("vertical",&fraction,{600,120},true);if(visible)ImGui::TextUnformatted("Top");
        if(ui::NextSplit())ImGui::TextUnformatted("Bottom");ui::EndSplit();end();
    }
    ui::set_scale(1);ui::set_touch(false);
    // O(visible rows), not O(total rows); return value must match callbacks.
    int maxRows=0;
    for(int f=0;f<3;++f){begin();int calls=0;int submitted=ui::VirtualList("list",100000,160,32,row,&calls);
        assert(calls==submitted&&calls<20);maxRows=std::max(maxRows,calls);end();}
    assert(maxRows>0);
    begin();assert(!ui::VirtualList("bad",10,0,32,row));assert(!ui::VirtualList("bad",10,32,0,row));end();
    begin();ui::Toasts("notifications",queue,2);end();
    std::printf("100000 rows: at most %d callbacks/frame; interaction and layout checks passed\n",maxRows);
    ImGui::DestroyContext();
}
