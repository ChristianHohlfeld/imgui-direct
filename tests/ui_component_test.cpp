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
    // Variant semantics must be noticeable, not three identical colors.
    for(ui::Mode theme:{ui::Mode::Light,ui::Mode::Dark}){
        ui::set_mode(theme);ui::begin_frame();
        for(ui::Variant kind:{ui::Variant::Primary,ui::Variant::Secondary,
                              ui::Variant::Neutral,ui::Variant::Outline,
                              ui::Variant::Ghost,ui::Variant::Soft,
                              ui::Variant::Error}){
            ui::Options opt;opt.variant=kind;
            const auto p=ui::button_visual(opt);
            auto diff=[](ui::Color a,ui::Color b){
                return std::fabs(a.r-b.r)+std::fabs(a.g-b.g)+std::fabs(a.b-b.b);
            };
            assert(diff(p.normal,p.hover)>.04f);
            assert(diff(p.normal,p.pressed)>.07f);
            opt.state.selected=true;
            const auto selected=ui::button_visual(opt);
            assert(selected.outline);
            opt.state.invalid=true;
            const auto invalid=ui::button_visual(opt);
            assert(invalid.outline);
            assert(diff(invalid.border,ui::palette().error)<.001f);
        }
    }
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
