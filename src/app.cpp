#include "app.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
struct State {
    int tab=0, clicks=0;
    bool animate=true, metrics=false, official=false;
    float amplitude=.70f, speed=1.0f, phase=0.f;
    float samples[128]{};
    char input[128]="Dear ImGui";
} s;
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
    bool mobile=io.DisplaySize.x<700.f;
    float margin=mobile?8.f:24.f;
    ImGui::SetNextWindowPos(ImVec2(margin,margin),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(std::max(260.f,io.DisplaySize.x-margin*2),
                                    std::max(220.f,io.DisplaySize.y-margin*2)),ImGuiCond_Always);
    ImGui::Begin("##direct",nullptr,ImGuiWindowFlags_NoTitleBar|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(ImVec4(.48f,.74f,1.f,1.f),"IMGUI / DIRECT");
    ImGui::TextDisabled("One C++20 UI - Native GPU / WebAssembly");
    ImGui::Separator();
    if(ImGui::Button("Overview")) s.tab=0;
    ImGui::SameLine();
    if(ImGui::Button("Controls")) s.tab=1;
    ImGui::SameLine();
    if(ImGui::Button("Architecture")) s.tab=2;
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
        ImGui::InputText("Text",s.input,sizeof(s.input));
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
    ImGui::TextDisabled("Identical src/app.cpp on all platforms.");
    ImGui::End();
    if(s.metrics) ImGui::ShowMetricsWindow(&s.metrics);
    if(s.official) ImGui::ShowDemoWindow(&s.official);
}
