#include "ui/theme.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace {
Mode currentMode=Mode::Dark;
Palette active=slateDark;
bool dark=true;
bool touch=false;
float fontScale=1.f;

ImVec4 v(Color c) {return {c.r,c.g,c.b,c.a};}
void apply() {
    // All colors and dimensions come from one source of truth. Never use
    // StyleColorsDark()/Light() per-widget, nor scale styles cumulatively.
    if(dark)ImGui::StyleColorsDark();else ImGui::StyleColorsLight();
    ImGuiStyle& s=ImGui::GetStyle();
    const Palette& p=active;
    auto set=[&](ImGuiCol n,Color c){s.Colors[n]=v(c);};
    set(ImGuiCol_WindowBg,p.base100);
    set(ImGuiCol_ChildBg,p.base200);
    set(ImGuiCol_PopupBg,p.base200);
    set(ImGuiCol_Text,p.baseContent);
    set(ImGuiCol_TextDisabled,blend(p.baseContent,p.base100,.44f));
    set(ImGuiCol_Border,p.base300);
    set(ImGuiCol_Separator,p.base300);
    set(ImGuiCol_FrameBg,p.base200);
    set(ImGuiCol_FrameBgHovered,blend(p.base200,p.primary,.13f));
    set(ImGuiCol_FrameBgActive,blend(p.base200,p.primary,.22f));
    set(ImGuiCol_Button,p.secondary);
    set(ImGuiCol_ButtonHovered,blend(p.secondary,p.primary,.15f));
    set(ImGuiCol_ButtonActive,blend(p.secondary,p.primary,.28f));
    set(ImGuiCol_CheckMark,p.primary);
    set(ImGuiCol_SliderGrab,p.primary);
    set(ImGuiCol_SliderGrabActive,blend(p.primary,p.baseContent,.14f));
    set(ImGuiCol_Header,p.base300);
    set(ImGuiCol_HeaderHovered,blend(p.base300,p.primary,.17f));
    set(ImGuiCol_HeaderActive,blend(p.base300,p.primary,.30f));
    set(ImGuiCol_Tab,p.base200);
    set(ImGuiCol_TabHovered,blend(p.base200,p.primary,.28f));
    set(ImGuiCol_TabSelected,p.base300);
    set(ImGuiCol_TabSelectedOverline,p.primary);
    set(ImGuiCol_TitleBg,p.base200);
    set(ImGuiCol_TitleBgActive,p.base200);
    set(ImGuiCol_ResizeGrip,p.primary);
    set(ImGuiCol_ResizeGripHovered,p.primary);
    set(ImGuiCol_ResizeGripActive,p.primary);
    set(ImGuiCol_NavCursor,p.primary);
    set(ImGuiCol_ScrollbarBg,p.base100);
    set(ImGuiCol_ScrollbarGrab,p.base300);
    set(ImGuiCol_ScrollbarGrabHovered,blend(p.base300,p.primary,.25f));
    set(ImGuiCol_ScrollbarGrabActive,blend(p.base300,p.primary,.35f));
    set(ImGuiCol_TableHeaderBg,p.base200);
    set(ImGuiCol_TableBorderStrong,p.base300);
    set(ImGuiCol_TableBorderLight,p.base300);
    set(ImGuiCol_TableRowBg,p.base100);
    set(ImGuiCol_TableRowBgAlt,p.base200);
    s.WindowRounding=12.f;
    s.ChildRounding=12.f;
    s.PopupRounding=9.f;
    s.FrameRounding=8.f;
    s.GrabRounding=7.f;
    s.ScrollbarRounding=7.f;
    s.WindowBorderSize=0.f;
    s.ChildBorderSize=1.f;
    s.FrameBorderSize=0.f;
    s.WindowPadding=ImVec2(touch?16.f:20.f,touch?15.f:18.f);
    s.FramePadding=ImVec2(touch?14.f:13.f,touch?13.f:9.f);
    s.ItemSpacing=ImVec2(11.f,touch?14.f:12.f);
    s.ItemInnerSpacing=ImVec2(8.f,6.f);
    s.ScrollbarSize=touch?20.f:14.f;
    s.GrabMinSize=touch?27.f:14.f;
    s.TouchExtraPadding=ImVec2(touch?5.f:0.f,touch?4.f:0.f);
    s.FontSizeBase=touch?18.f:16.f;
    s.FontScaleMain=fontScale;
    s.ScaleAllSizes(fontScale);
}
}
Mode mode(){return currentMode;}
const Palette& palette(){return active;}
void set_mode(Mode next) {
    if(next==Mode::Custom)return;
    currentMode=next;
    dark=next==Mode::Dark;
    active=dark?slateDark:slateLight;
    apply();
}
void set_custom_palette(const Palette& custom,bool darkBase) {
    currentMode=Mode::Custom; active=custom;dark=darkBase;apply();
}
void initialize(){set_mode(Mode::Dark);}
void set_touch(bool value){if(touch==value)return;touch=value;apply();}
bool is_touch(){return touch;}
float scale(){return fontScale;}
void set_scale(float value){
    if(!std::isfinite(value))return;
    fontScale=std::clamp(value,.8f,2.f);apply();
}
void zoom(float factor){if(std::isfinite(factor)&&factor>0.f)set_scale(fontScale*factor);}
}
