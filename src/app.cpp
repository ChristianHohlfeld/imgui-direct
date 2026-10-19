#include "app.hpp"
#include "ui/components.hpp"
#include "ui/theme.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#ifdef __EMSCRIPTEN__
extern "C" void browser_input_rect(float,float,float,float);
#endif

namespace {
struct State {
    int tab=0, clicks=0;
    bool animate=true, metrics=false, official=false, notifications=true;
    bool newsletter=false, light=false;
    float amplitude=.70f, speed=1.f, phase=0.f;
    float samples[128]{};
    char input[160]="Type here ...";
} s;
float safeTop=0,safeRight=0,safeBottom=0,safeLeft=0;
float keyboard=0.f;
bool requestTextFocus=false;

void Overview(bool compact) {
    ui::Heading("One UI. Every screen.");
    ui::Muted("Native Dear ImGui components. Shared C++ state, no web framework.");
    ui::Badge("WASM",ui::Status::Info);
    ImGui::SameLine();ui::Badge("Native",ui::Status::Success);
    ImGui::SameLine();ui::Badge("Zero UI dependencies",ui::Status::Info);
    ImGui::Spacing();
    ImGui::Text("GPU renderer  |  %.0f FPS",ImGui::GetIO().Framerate);
    ImGui::TextDisabled("Viewport: %.0f x %.0f",ImGui::GetIO().DisplaySize.x,ImGui::GetIO().DisplaySize.y);
    for(int i=0;i<127;++i)s.samples[i]=s.samples[i+1];
    if(s.animate)s.phase+=std::min(ImGui::GetIO().DeltaTime,.1f)*s.speed*2.f;
    s.samples[127]=.5f+.36f*s.amplitude*(std::sin(s.phase)+.3f*std::sin(3.f*s.phase));
    ImGui::PlotLines("##wave",s.samples,128,0,"Live C++ waveform",0.f,1.f,
                     ImVec2(-1.f,compact?110.f:185.f));
    if(ui::Button("Explore components",ui::Variant::Primary,
          ImVec2(compact?-1.f:205.f,0)))s.tab=1;
    if(!compact){ImGui::SameLine();if(ui::Button("Architecture",ui::Variant::Outline))s.tab=2;}
    ImGui::Spacing();
    ui::Alert("One shared implementation",
              "The same controls run on Windows, Linux, macOS, Android, iOS and WebAssembly.",
              ui::Status::Info);
}
void Components(bool compact) {
    ui::Heading("Component foundation");
    ui::Muted("First principles: identity, action, input, choice, feedback and layout.");
    ImGui::Spacing();
    const float total=ImGui::GetContentRegionAvail().x;
    const bool columns=!compact && total>780.f;
    if(columns)ImGui::Columns(2,"##component-columns",false);
    if(ui::BeginCard("##actions",(compact?410.f:330.f)*ui::scale())) {
        ui::Heading("Actions");
        ui::Muted("Primary, secondary, outline, ghost and destructive.");
        if(ui::Button("Primary action",ui::Variant::Primary))++s.clicks;
        if(!compact)ImGui::SameLine();
        ui::Button("Secondary",ui::Variant::Secondary);
        ui::Button("Outline",ui::Variant::Outline);
        if(!compact)ImGui::SameLine();
        ui::Button("Ghost",ui::Variant::Ghost);
        ui::Button("Delete",ui::Variant::Error);
        if(!compact)ImGui::SameLine();
        ui::Button("Disabled",ui::Variant::Neutral,{0,0},false);
        ImGui::Text("Clicked: %d",s.clicks);
        ui::Badge("Ready",ui::Status::Success);
        ImGui::SameLine();ui::Badge("Notice",ui::Status::Warning);
    }
    ui::EndCard();
    if(columns)ImGui::NextColumn();
    else ImGui::Spacing();
    if(ui::BeginCard("##fields",(compact?390.f:330.f)*ui::scale())) {
        ui::Heading("Inputs");
        ui::Muted("Keyboard, clipboard, touch and hardware keys.");
        if(requestTextFocus){ImGui::SetKeyboardFocusHere();requestTextFocus=false;}
        ui::Input("Your text",s.input,sizeof(s.input),"Write something");
#ifdef __EMSCRIPTEN__
        const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        browser_input_rect(a.x,a.y,b.x-a.x,b.y-a.y);
#endif
        ui::Slider("Intensity",&s.amplitude,0.f,1.f);
        ui::Checkbox("Animate waveform",&s.animate);
        ui::Switch("Notifications",&s.notifications);
    }
    ui::EndCard();
    if(columns)ImGui::Columns(1);
    ImGui::Spacing();
    ui::Alert("Saved locally in memory",
              "Controls share one C++ state object. This demo does not send input to a server.",
              ui::Status::Success);
}
void System() {
    ui::Heading("Design system");
    ui::Muted("Semantic tokens, not per-screen colors. One understated design.");
    ImGui::Spacing();
    if(ui::BeginCard("##themes",210.f*ui::scale())) {
        ui::Heading("Theme");
        ui::Muted("Slate: Dark / Light. Custom palettes are C++ data.");
        ui::Switch("Light appearance",&s.light);
        // Switch is a standard ImGui button with keyboard focus and native input.
        const ui::Mode desired=s.light?ui::Mode::Light:ui::Mode::Dark;
        if(ui::mode()!=desired)ui::set_mode(desired);
        ui::Divider();
        ui::Badge("Primary",ui::Status::Info);
        ImGui::SameLine();ui::Badge("Success",ui::Status::Success);
        ImGui::SameLine();ui::Badge("Warning",ui::Status::Warning);
        ImGui::SameLine();ui::Badge("Error",ui::Status::Error);
    }
    ui::EndCard();
    ImGui::Spacing();
    ui::Muted("C++20 + Dear ImGui. Win32 / X11 / UIKit + Metal / Android NDK / WASM.");
    if(ui::Button("Show Dear ImGui demo",ui::Variant::Outline))s.official=!s.official;
    ImGui::SameLine();if(ui::Button("Metrics",ui::Variant::Ghost))s.metrics=!s.metrics;
}
}
void app::setup() {
    ui::initialize();
}
void app::set_touch(bool enabled) {
    if(enabled)ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_IsTouchScreen;
    else ImGui::GetIO().ConfigFlags&=~ImGuiConfigFlags_IsTouchScreen;
    ui::set_touch(enabled);
}
void app::safe_area(float top,float right,float bottom,float left) {
    safeTop=top;safeRight=right;safeBottom=bottom;safeLeft=left;
}
void app::keyboard_height(float value){keyboard=std::max(0.f,value);}
void app::request_text_focus(){requestTextFocus=true;s.tab=1;}
void app::zoom(float ratio){ui::zoom(ratio);}
void app::draw() {
    auto& io=ImGui::GetIO();
    if(io.DisplaySize.x<=0.f||io.DisplaySize.y<=0.f)return;
    const bool compact=ui::is_touch() || io.DisplaySize.x<720.f;
    const float margin=compact?7.f:20.f;
    const float x=margin+safeLeft,y=margin+safeTop;
    const float width=std::max(225.f,io.DisplaySize.x-x-margin-safeRight);
    const float height=std::max(160.f,io.DisplaySize.y-y-margin-safeBottom-keyboard);
    ImGui::SetNextWindowPos({x,y},ImGuiCond_Always);
    ImGui::SetNextWindowSize({width,height},ImGuiCond_Always);
    const ImGuiWindowFlags flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##app",nullptr,flags);
    ui::Heading("IMGUI / DIRECT");
    ui::Muted("Native C++ components");
    ImGui::Spacing();
    // Deliberately keep the appearance switch reachable with one hand on phones.
    const char* desktopNames[]={"Overview","Components","Theme"};
    const char* mobileNames[]={"Home","UI","Theme"};
    ui::Tabs("main-tabs",compact?mobileNames:desktopNames,3,&s.tab);
    ui::Divider();
    if(s.tab==0) Overview(compact);
    else if(s.tab==1) Components(compact);
    else System();
#ifdef __EMSCRIPTEN__
    if(s.tab!=1)browser_input_rect(0,0,0,0);
#endif
    ImGui::Spacing();
    ui::Divider();
    ui::Muted("Dear ImGui is the fixed UI core  -  C++20  -  Direct GPU");
    ImGui::End();
    if(s.metrics)ImGui::ShowMetricsWindow(&s.metrics);
    if(s.official)ImGui::ShowDemoWindow(&s.official);
}
