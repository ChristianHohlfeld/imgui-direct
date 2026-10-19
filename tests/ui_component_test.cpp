#include "ui/components.hpp"
#include "ui/theme.hpp"
#include <cassert>
#include <cmath>
int main(){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ui::initialize();
    const float desktop=ui::control_height(ui::Size::MD);
    assert(desktop>=38.f);
    const auto dark=ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    ui::set_mode(ui::Mode::Light);
    ui::begin_frame();
    const auto light=ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    assert(light.x>dark.x);
    ui::set_touch(true);
    assert(ui::control_height(ui::Size::XS)>=44.f);
    assert(ui::control_height(ui::Size::MD)>=48.f);
    assert(ui::control_height(ui::Size::XL)>=60.f);
    ui::set_scale(1.5f);
    assert(ui::control_height(ui::Size::MD)>=72.f);
    ui::zoom(.001f); // bounded pinch zoom
    assert(ui::scale()>=.8f);
    ui::zoom(999.f);
    assert(ui::scale()<=2.f);
    auto custom=ui::slateLight;
    custom.primary=ui::hex(0x9c27b0);
    ui::set_custom_palette(custom,false);
    ui::begin_frame();
    assert(ui::mode()==ui::Mode::Custom);
    assert(ui::palette().primary.r==custom.primary.r);
    ImGui::DestroyContext();
}
