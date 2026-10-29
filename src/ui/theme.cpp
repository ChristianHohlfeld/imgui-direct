#include "ui/theme.hpp"
#ifdef UI_EMBED_FONT
#include "ui/font_data.hpp"
#endif
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace {
Mode currentMode=Mode::Dark;
Palette active=daisyDark;
bool dark=true;
bool touch=false;
float fontScale=1.f;
bool themeDirty=false;

ImVec4 v(Color c) {return {c.r,c.g,c.b,c.a};}
void apply() {
    themeDirty=false;
    // All colors and dimensions come from one source of truth. Never use
    // StyleColorsDark()/Light() per-widget, nor scale styles cumulatively.
    if(dark)ImGui::StyleColorsDark();else ImGui::StyleColorsLight();
    ImGuiStyle& s=ImGui::GetStyle();
    const Palette& p=active;
    auto set=[&](ImGuiCol n,Color c){s.Colors[n]=v(c);};
    set(ImGuiCol_WindowBg,p.base200);
    set(ImGuiCol_ChildBg,p.base100);
    set(ImGuiCol_PopupBg,p.base100);
    set(ImGuiCol_Text,p.baseContent);
    set(ImGuiCol_TextDisabled,blend(p.baseContent,p.base100,.48f));
    set(ImGuiCol_Border,p.base300);
    set(ImGuiCol_Separator,p.base300);
    set(ImGuiCol_FrameBg,p.base100);
    set(ImGuiCol_FrameBgHovered,blend(p.base100,p.primary,.07f));
    set(ImGuiCol_FrameBgActive,blend(p.base100,p.primary,.12f));
    set(ImGuiCol_Button,p.base200);
    set(ImGuiCol_ButtonHovered,blend(p.base200,p.baseContent,.09f));
    set(ImGuiCol_ButtonActive,blend(p.base200,p.baseContent,.17f));
    set(ImGuiCol_CheckMark,p.primary);
    set(ImGuiCol_SliderGrab,p.primary);
    set(ImGuiCol_SliderGrabActive,blend(p.primary,p.baseContent,.14f));
    set(ImGuiCol_Header,p.base300);
    set(ImGuiCol_HeaderHovered,blend(p.base300,p.primary,.17f));
    set(ImGuiCol_HeaderActive,blend(p.base300,p.primary,.30f));
    set(ImGuiCol_Tab,p.base100);
    set(ImGuiCol_TabHovered,blend(p.base100,p.primary,.10f));
    set(ImGuiCol_TabSelected,p.base200);
    set(ImGuiCol_TabSelectedOverline,p.primary);
    set(ImGuiCol_TitleBg,p.base100);
    set(ImGuiCol_TitleBgActive,p.base100);
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
    s.WindowRounding=0.f;
    s.ChildRounding=p.metrics.radiusBox;
    s.PopupRounding=p.metrics.radiusBox;
    s.FrameRounding=p.metrics.radiusField;
    s.GrabRounding=p.metrics.radiusSelector;
    s.ScrollbarRounding=p.metrics.radiusSelector;
    s.WindowBorderSize=0.f;
    s.ChildBorderSize=p.metrics.border;
    s.PopupBorderSize=p.metrics.border;
    s.FrameBorderSize=p.metrics.border;
    s.WindowPadding=ImVec2(touch?15.f:20.f,touch?14.f:20.f);
    s.FramePadding=ImVec2(touch?12.f:13.f,touch?8.f:8.f);
    s.ItemSpacing=ImVec2(10.f,touch?12.f:12.f);
    s.ItemInnerSpacing=ImVec2(8.f,6.f);
    s.ScrollbarSize=touch?12.f:10.f;
    s.GrabMinSize=touch?21.f:14.f;
    s.TouchExtraPadding=ImVec2(touch?5.f:0.f,touch?4.f:0.f);
    s.FontSizeBase=touch?16.f:16.f;
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
    active=dark?daisyDark:daisyLight;
    themeDirty=true;
}
void set_custom_palette(const Palette& custom,bool darkBase) {
    currentMode=Mode::Custom; active=custom;dark=darkBase;themeDirty=true;
}
void initialize(){
    ImGuiIO& io=ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // The default embedded Proggy fonts are programmer fonts and do not match
    // a normal product UI. Select Inter explicitly; simply adding the font is
    // insufficient because ImGui may keep its previously created font.
    #ifdef UI_EMBED_FONT
    ImFontConfig config{};
    config.FontDataOwnedByAtlas=false;
    config.PixelSnapH=false;
    config.OversampleH=3;
    config.OversampleV=2;
    io.FontDefault=io.Fonts->AddFontFromMemoryTTF(
        (void*)inter_font_data,inter_font_size,16.f,&config);
    IM_ASSERT(io.FontDefault != nullptr);
    #else
    // Embedders own font selection; do not replace an existing engine atlas.
    if(io.Fonts->Fonts.empty())io.FontDefault=io.Fonts->AddFontDefault();
    #endif
    currentMode=Mode::Dark;
    dark=true;
    active=daisyDark;
    apply();
}
void begin_frame(){if(themeDirty)apply();}
void set_touch(bool value){if(touch==value)return;touch=value;apply();}
bool is_touch(){return touch;}
float scale(){return fontScale;}
void set_scale(float value){
    if(!std::isfinite(value))return;
    fontScale=std::clamp(value,.8f,2.f);apply();
}
void zoom(float factor){if(std::isfinite(factor)&&factor>0.f)set_scale(fontScale*factor);}
}
