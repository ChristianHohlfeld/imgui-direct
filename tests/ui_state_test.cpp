#include "state_gallery.hpp"
#include "ui/field_detail.hpp"
#include "imgui_internal.h"
#include <cassert>
#include <cstdio>
#include <cmath>
namespace {
void begin(){
 ui::begin_frame();ImGui::NewFrame();ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({700,1100});
 ImGui::Begin("review-test",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoTitleBar);
}
void end(){ImGui::End();ImGui::Render();}
void frame(){begin();review::preview();end();}
void click(ImVec2 p){
 auto& io=ImGui::GetIO();io.AddMousePosEvent(p.x,p.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();
}
}
int main(){
 IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={700,1100};io.DeltaTime=1.f/60;
 ui::initialize();unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
 int cases=0;
 for(bool touch:{false,true})for(auto mode:{ui::Mode::Light,ui::Mode::Dark}){
  ui::set_touch(touch);ui::set_mode(mode);
  for(int c=0;c<review::count();++c)for(int s=0;s<static_cast<int>(review::State::Count);++s){
   auto state=static_cast<review::State>(s);if(!review::supports(c,state))continue;
   review::select(c,state);begin();
   const int colors=GImGui->ColorStack.Size,styles=GImGui->StyleVarStack.Size,ids=ImGui::GetCurrentWindow()->IDStack.Size,disabled=GImGui->DisabledStackSize;
   review::preview();
   assert(GImGui->ColorStack.Size==colors&&GImGui->StyleVarStack.Size==styles);
   assert(ImGui::GetCurrentWindow()->IDStack.Size==ids&&GImGui->DisabledStackSize==disabled);
   end();++cases;
  }
 }
 ui::set_touch(false);ui::set_mode(ui::Mode::Light);
 // Real clicks and queued text must not mutate disabled/readonly/loading fields.
 for(int c=0;c<=13;++c)for(auto s:{review::State::Disabled,review::State::ReadOnly,review::State::Loading}){
  review::select(c,s);frame();frame();auto info=review::primary();
  assert(info.maximum.x>info.minimum.x&&info.maximum.y>info.minimum.y);
  click({info.minimum.x+15,(info.minimum.y+info.maximum.y)*.5f});
  io.AddInputCharactersUTF8("Z");frame();assert(review::changes()==0);
 }
 // Labels are part of switch hit target; helper text cannot replace its snapshot.
 review::select(6,review::State::Normal);frame();frame();auto info=review::primary();
 assert(info.maximum.x-info.minimum.x>100);click({info.maximum.x-8,(info.minimum.y+info.maximum.y)*.5f});assert(review::changes()==1);
 // Hover and active are real input transitions. Touch must not retain hover.
 review::select(0,review::State::Normal);frame();frame();info=review::primary();
 io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(info.minimum.x+20,info.minimum.y+15);frame();assert(review::primary().interaction.hovered);
 io.AddMouseButtonEvent(0,true);frame();assert(review::primary().interaction.held);
 io.AddMouseButtonEvent(0,false);frame();assert(review::changes()==1);
 io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);io.AddMousePosEvent(info.minimum.x+21,info.minimum.y+16);frame();frame();assert(!review::primary().interaction.hovered);
 io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
 // Composite native InputInt submits an input and step buttons under one group.
 // Hovering an actual child must be reported by the public component snapshot.
 review::select(9,review::State::Normal);frame();frame();info=review::primary();
 assert(info.maximum.x>info.minimum.x&&info.maximum.y>info.minimum.y);
 io.AddMousePosEvent(info.minimum.x+20,(info.minimum.y+info.maximum.y)*.5f);
 frame();frame();
 assert(review::primary().interaction.hovered);
 // Actual keyboard focus/activation, with native help content following field.
 begin();bool checked=false;ui::Field f;f.help="Help after actual control";ui::Checkbox("check",&checked,f);
 const auto checkbox=ui::last_control();assert(checkbox.id==ImGui::GetItemID());
 ImGui::SetNavID(checkbox.id,ImGuiNavLayer_Main,ImGui::GetCurrentFocusScope(),ImRect(checkbox.minimum,checkbox.maximum));ImGui::SetNavCursorVisible(true);end();
 io.AddKeyEvent(ImGuiKey_Space,true);begin();ui::Checkbox("check",&checked,f);end();
 io.AddKeyEvent(ImGuiKey_Space,false);begin();ui::Checkbox("check",&checked,f);end();assert(checked);
 // Traverse every enabled primary target with real Tab events, including child panes.
 for(int c=0;c<review::count();++c){
  if(!review::component(c).interactive)continue;
  review::select(c,review::State::Normal);frame();frame();auto target=review::primary();
  if(target.maximum.x<=target.minimum.x||target.interaction.disabled)continue;
  bool focused=false;
  for(int attempt=0;attempt<24&&!focused;++attempt){
   io.AddKeyEvent(ImGuiKey_Tab,true);frame();io.AddKeyEvent(ImGuiKey_Tab,false);frame();focused=review::primary().interaction.focused;
  }
  if(!focused)std::fprintf(stderr,"Tab failed: %d %s\n",c,review::component(c).name);
  assert(focused);
 }
 // Every size affects check/radio field geometry; touch minimums remain enforced.
 for(bool touch:{false,true})for(int s=0;s<5;++s){
  ui::set_touch(touch);begin();f={};f.size=static_cast<ui::Size>(s);bool value=false;ui::Checkbox("sized",&value,f);
  auto r=ui::last_control();assert(r.maximum.y-r.minimum.y>=ui::control_height(f.size)-.01f);end();
 }
 // Error precedence over success is explicit; readonly differs from normal visually.
 f={};const auto plain=ui::field_visual(f);f.readOnly=true;const auto locked=ui::field_visual(f);assert(plain.normal.r!=locked.normal.r);
 f.invalid=true;f.validation=ui::ValidationState::Success;assert(ui::field_visual(f).border.r==ui::palette().error.r);
 std::printf("%d component/state/theme/input renders; locking, snapshots, label hits, hover, press and keyboard passed\n",cases);
 ImGui::DestroyContext();
}
