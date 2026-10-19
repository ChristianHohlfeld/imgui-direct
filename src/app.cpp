#include "app.hpp"
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
    bool animate=true, metrics=false, official=false;
    float amplitude=.70f, speed=1.0f, phase=0.f;
    float samples[128]{};
    char input[128]="Dear ImGui";
} s;
bool touchMode=false;
float uiScale=1.f;
float safeTop=0,safeRight=0,safeBottom=0,safeLeft=0;
float keyboard=0.f;
bool requestTextFocus=false;
}
void app::set_touch(bool touch) {
    if(touchMode==touch)return;
    touchMode=touch;
    if(touch) ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_IsTouchScreen;
    ImGuiStyle& st=ImGui::GetStyle();
    if(touch) {
        st.FramePadding=ImVec2(14,11);
        st.ItemSpacing=ImVec2(10,11);
        st.GrabMinSize=24.f;
        st.TouchExtraPadding=ImVec2(3,4);
        st.ScrollbarSize=20.f;
        st.FontSizeBase=19.f;
    }
}
void app::safe_area(float top,float right,float bottom,float left) {
    safeTop=top;safeRight=right;safeBottom=bottom;safeLeft=left;
}
void app::keyboard_height(float height) {keyboard=std::max(0.f,height);}
void app::request_text_focus() {requestTextFocus=true;}
void app::zoom(float ratio) {
    if(!std::isfinite(ratio)||ratio<=0.f)return;
    const float scale=std::clamp(uiScale*ratio,.8f,1.8f);
    const float delta=scale/uiScale;
    if(std::abs(delta-1.f)<.001f)return;
    auto& st=ImGui::GetStyle();
    st.ScaleAllSizes(delta);
    st.FontScaleMain*=delta;
    uiScale=scale;
}
void app::setup() {
    ImGui::StyleColorsDark();
    ImGuiStyle& x=ImGui::GetStyle();
    x.WindowRounding=10.f; x.ChildRounding=8.f; x.FrameRounding=7.f;
    x.FramePadding=ImVec2(11.f,8.f); x.ItemSpacing=ImVec2(9.f,9.f);
    x.Colors[ImGuiCol_WindowBg]=ImVec4(.075f,.09f,.13f,1.f);
}
void app::draw() {
    ImGuiIO& io=ImGui::GetIO();
    if(io.DisplaySize.x<=0||io.DisplaySize.y<=0) return;
    bool mobile=touchMode||io.DisplaySize.x<700.f;
    float margin=mobile?8.f:24.f;
    float x=margin+safeLeft,y=margin+safeTop;
    float width=std::max(240.f,io.DisplaySize.x-x-margin-safeRight);
    float height=std::max(160.f,io.DisplaySize.y-y-margin-safeBottom-keyboard);
    ImGui::SetNextWindowPos(ImVec2(x,y),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always);
    ImGui::Begin("##direct",nullptr,ImGuiWindowFlags_NoTitleBar|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(ImVec4(.48f,.74f,1.f,1.f),"IMGUI / DIRECT");
    ImGui::TextDisabled("One C++20 UI - Native GPU / WebAssembly");
    ImGui::Separator();
    float buttonWidth=(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x*2.f)/3.f;
    if(ImGui::Button("Overview",ImVec2(buttonWidth,0))) s.tab=0;
    ImGui::SameLine();
    if(ImGui::Button("Controls",ImVec2(buttonWidth,0))) s.tab=1;
    ImGui::SameLine();
    if(ImGui::Button("System",ImVec2(buttonWidth,0))) s.tab=2;
    ImGui::Separator();
    if(s.tab==0) {
        ImGui::Text("GPU-rendered UI  |  %.1f FPS",io.Framerate);
        ImGui::TextDisabled("Viewport: %.0f x %.0f",io.DisplaySize.x,io.DisplaySize.y);
        for(int i=0;i<127;++i) s.samples[i]=s.samples[i+1];
        if(s.animate) s.phase+=std::min(io.DeltaTime,.1f)*s.speed*2.f;
        s.samples[127]=.5f+.36f*s.amplitude*(std::sin(s.phase)+.3f*std::sin(s.phase*3.f));
        ImGui::PlotLines("##plot",s.samples,128,0,"Live C++ waveform",0.f,1.f,
                         ImVec2(-1.f,mobile?115.f:210.f));
        ImGui::Checkbox("Animate",&s.animate);
    } else if(s.tab==1) {
        if(ImGui::Button("Click me")) ++s.clicks;
        ImGui::SameLine(); ImGui::Text("Counter: %d",s.clicks);
        ImGui::SliderFloat("Amplitude",&s.amplitude,0.f,1.f);
        ImGui::SliderFloat("Speed",&s.speed,0.f,4.f);
        if(requestTextFocus) {ImGui::SetKeyboardFocusHere();requestTextFocus=false;}
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##native-text",s.input,sizeof(s.input));
#ifdef __EMSCRIPTEN__
        auto inputMin=ImGui::GetItemRectMin();
        auto inputMax=ImGui::GetItemRectMax();
        browser_input_rect(inputMin.x,inputMin.y,inputMax.x-inputMin.x,inputMax.y-inputMin.y);
#endif
        ImGui::TextDisabled("Native keyboard / paste / hardware keys");
        ImGui::Separator();
        ImGui::TextWrapped("Try tap, drag, long press for context, two-finger pan, pinch to zoom.");
        ImGui::Checkbox("ImGui metrics",&s.metrics);
        ImGui::Checkbox("Official demo",&s.official);
    } else {
        ImGui::BulletText("Dear ImGui is the permanent UI core");
        ImGui::BulletText("Direct platform APIs, no SDL/GLFW/Sokol");
        ImGui::BulletText("Rendering: OpenGL/WebGL2; Metal on Apple");
        ImGui::BulletText("Browser: same C++ code, Emscripten WASM");
        ImGui::TextWrapped("Only third-party source dependency: ocornut/imgui.");
    }
    ImGui::Separator();
    ImGui::TextDisabled("Identical C++ UI on every platform.  Zoom %.0f%%",uiScale*100.f);
#ifdef __EMSCRIPTEN__
    if(s.tab!=1)browser_input_rect(0,0,0,0);
#endif
    ImGui::End();
    if(s.metrics) ImGui::ShowMetricsWindow(&s.metrics);
    if(s.official) ImGui::ShowDemoWindow(&s.official);
}
