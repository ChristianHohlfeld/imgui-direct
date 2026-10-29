#include "ui/essentials.hpp"
#include "ui/extended.hpp"
#include "imgui_internal.h"
#include <cassert>
#include <cstdio>

namespace {
void begin(){
    ui::begin_frame();ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0});
    ImGui::SetNextWindowSize({720,920});
    ImGui::Begin("essentials",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
}
void end(){ImGui::End();ImGui::Render();}
ImVec2 center(){
    auto c=ui::last_control();return {(c.minimum.x+c.maximum.x)*.5f,(c.minimum.y+c.maximum.y)*.5f};
}
template<class Draw> void click(ImVec2 point,Draw draw){
    auto& io=ImGui::GetIO();
    io.AddMousePosEvent(point.x,point.y);draw();
    io.AddMouseButtonEvent(0,true);draw();
    io.AddMouseButtonEvent(0,false);draw();
}
}
int main(){
    IMGUI_CHECKVERSION();ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={720,920};io.DeltaTime=1.f/60.f;
    ui::initialize();
    unsigned char* pixels=nullptr;int width=0,height=0;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    assert(pixels&&width>0&&height>0);
    for(bool touch:{false,true})for(auto mode:{ui::Mode::Light,ui::Mode::Dark}){
        ui::set_touch(touch);ui::set_mode(mode);
        begin();
        const int colorStack=GImGui->ColorStack.Size;
        const int styleStack=GImGui->StyleVarStack.Size;
        ui::Badge("Success",ui::BadgeOptions{ui::Tone::Success,ui::Surface::Solid});
        ui::Badge("Soft",ui::BadgeOptions{ui::Tone::Accent,ui::Surface::Soft});
        ui::Badge("Outline",ui::BadgeOptions{ui::Tone::Primary,ui::Surface::Outline});
        ui::Alert("Status","Everything is fine",ui::AlertOptions{ui::Tone::Info,ui::Surface::Soft});
        ui::Progress("Loading",.5f);
        if(ui::BeginCard("card",ui::CardOptions{ui::Size::SM,true})){ui::Heading("Properties");}
        ui::EndCard();
        int stars=3,pick=1;const char* labels[]={"Choose","Edit","View"};
        assert(!ui::Rating("Rating",&stars,5));
        assert(!ui::Segmented("mode",labels,&pick));
        assert(!ui::Link("Open details",ui::Tone::Primary));
        ui::ChatBubble("Native, interactive UI.",false,ui::Tone::Base,"System","Now");
        ui::ChatBubble("Scene loaded.",true,ui::Tone::Primary);
        assert(GImGui->ColorStack.Size==colorStack&&GImGui->StyleVarStack.Size==styleStack);
        end();
    }
    ui::set_touch(false);ui::set_mode(ui::Mode::Light);
    int links=0;
    auto link=[&](){begin();if(ui::Link("Open details",ui::Tone::Primary))++links;end();};
    link();const auto linkCenter=center();
    click(linkCenter,link);assert(links==1);
    auto disabledLink=[&](){begin();if(ui::Link("Open details",ui::Tone::Primary,true))++links;end();};
    click(linkCenter,disabledLink);assert(links==1);
    const char* names[]={"Choose","Edit","View"};
    int selection=0,changes=0;
    auto segmented=[&](){begin();if(ui::Segmented("segmented",names,&selection))++changes;end();};
    segmented();const auto lastSegment=center();
    click(lastSegment,segmented);assert(selection==2&&changes==1);
    ui::Options disabled;disabled.state.disabled=true;
    auto lockedSegmented=[&](){begin();if(ui::Segmented("segmented",names,&selection,disabled))++changes;end();};
    click(lastSegment,lockedSegmented);assert(selection==2&&changes==1);
    int rating=0,ratingChanges=0;
    auto stars=[&](){begin();if(ui::Rating("rating",&rating,5))++ratingChanges;end();};
    stars();const auto fifth=center();
    click(fifth,stars);assert(rating==5&&ratingChanges==1);
    auto readonlyStars=[&](){begin();if(ui::Rating("rating",&rating,5,ui::Size::MD,true))++ratingChanges;end();};
    click(fifth,readonlyStars);assert(rating==5&&ratingChanges==1);
    std::puts("daisy essentials: rendering, scope balance and native mouse/locking passed");
    ImGui::DestroyContext();
}
