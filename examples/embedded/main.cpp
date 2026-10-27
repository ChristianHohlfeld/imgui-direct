#include "ui/extended.hpp"
// Headless embedding smoke test. A real engine supplies input + GPU backends.
int main(){
    IMGUI_CHECKVERSION();ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={800,600};io.DeltaTime=1.f/60;
    // Host owns its font atlas; library-only integration must preserve it.
    auto* font=io.Fonts->AddFontDefault();io.FontDefault=font;ui::initialize();
    if(io.FontDefault!=font)return 1;
    unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    float position[3]={0,1,0};
    ImGui::NewFrame();ui::begin_frame();ImGui::Begin("Properties");
    ui::Vector3("Position",position);ImGui::End();ImGui::Render();
    ui::SetInputObserver(nullptr);ImGui::DestroyContext();return 0;
}
