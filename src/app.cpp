#include "app.hpp"
#include "ui/components.hpp"
#include "ui/theme.hpp"
#include "ui/scroll.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

#ifdef __EMSCRIPTEN__
extern "C" void browser_input_rect(float,float,float,float);
#endif

namespace {
struct Demo {
    int page=0, choice=0, plan=1, clicks=0, listSelected=-1;
    bool dark=false, notify=true, agreed=true, invalid=false, modal=false;
    float volume=.65f, progress=.72f;
    char name[128]="Alex Morgan";
    char email[128]="alex@example.com";
    char note[320]="Ein kurzer Gedanke ...";
} d;
float safeTop=0,safeRight=0,safeBottom=0,safeLeft=0,keyboardHeight=0;
bool focusInput=false;
void space(float n=8.f){ImGui::Dummy({0,n*ui::scale()});}
void introduction(const char* title,const char* description){
    ui::Heading(title);ui::Muted(description);space(8);
}
void buttons(bool narrow){
    if(ui::BeginCard("##buttons")){
        introduction("Buttons","Eine klare Hierarchie für jede Aktion.");
        ui::Options opt;
        const float half=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*.5f;
        const ImVec2 size=narrow?ImVec2(half,0.f):ImVec2(0.f,0.f);
        if(ui::Button("Primär",opt,size))++d.clicks;
        ImGui::SameLine();
        opt.variant=ui::Variant::Secondary;
        if(ui::Button("Sekundär",opt,size))++d.clicks;
        space(3);
        opt.variant=ui::Variant::Outline;
        ui::Button("Outline",opt,size);
        ImGui::SameLine();
        opt.variant=ui::Variant::Ghost;
        ui::Button("Ghost",opt,size);
        space(3);
        opt.variant=ui::Variant::Soft;
        ui::Button("Soft",opt,size);
        ImGui::SameLine();
        opt.variant=ui::Variant::Error;
        ui::Button("Entfernen",opt,size);
        space(6);ui::Divider();space(4);
        ui::Muted("Zustände");
        opt.variant=ui::Variant::Secondary;
        opt.state.loading=true;
        ui::Button("Lädt ...",opt,size);
        ImGui::SameLine();
        opt.state={};opt.state.disabled=true;
        ui::Button("Deaktiviert",opt,size);
        space(3);
        opt.state={};opt.state.selected=true;
        opt.variant=ui::Variant::Outline;
        ui::Button("Ausgewählt",opt,size);
    }
    ui::EndCard();
}
void fields(bool narrow){
    if(ui::BeginCard("##fields")){
        introduction("Formulare","Konsistente Eingabe- und Auswahlfelder.");
        ui::Field required;required.required=true;
        required.invalid=d.invalid;
        required.help=d.invalid?"Bitte einen Namen eingeben.":nullptr;
        if(focusInput){ImGui::SetKeyboardFocusHere();focusInput=false;}
        ui::Input("Name",d.name,sizeof(d.name),"Vollständiger Name",required);
#ifdef __EMSCRIPTEN__
        {ImVec2 a,b;ui::LastInputBounds(a,b);
         browser_input_rect(a.x,a.y,b.x-a.x,b.y-a.y);}
#endif
        ui::Input("E-Mail",d.email,sizeof(d.email),"name@beispiel.de");
        static const char* options[]={"Privat","Team","Unternehmen"};
        ui::Select("Bereich",options,3,&d.choice);
        space();
        ui::Checkbox("Ich stimme zu",&d.agreed);
        ui::Switch("Benachrichtigungen",&d.notify);
        space(10);
        ui::Options opt;opt.fullWidth=narrow;
        if(ui::Button("Bestätigen",opt)){
            d.invalid=std::strlen(d.name)==0;
            if(!d.invalid)d.modal=true;
        }
    }
    ui::EndCard();
}
void choices(){
    if(ui::BeginCard("##choices")){
        introduction("Auswahl","Natürliche Eingaben für Maus und Touch.");
        ui::Radio("Standard",0,&d.plan);
        ui::Radio("Erweitert",1,&d.plan);
        space();
        ui::Slider("Lautstärke",&d.volume,0.f,1.f,"%.0f");
        space();
        ui::Field notes;notes.help="Optional.";
        ui::Textarea("Notizen",d.note,sizeof(d.note),notes,3);
    }
    ui::EndCard();
}
void feedback(bool narrow){
    if(ui::BeginCard("##feedback")){
        introduction("Feedback","Unaufdringliche Statusmeldungen.");
        ui::Badge("Info",ui::Status::Info);
        ImGui::SameLine();ui::Badge("Erfolg",ui::Status::Success);
        if(narrow)space(3);else ImGui::SameLine();
        ui::Badge("Hinweis",ui::Status::Warning);
        ImGui::SameLine();ui::Badge("Fehler",ui::Status::Error);
        space(10);
        ui::Alert("Erfolgreich gespeichert",
                  "Deine Änderungen wurden übernommen.",ui::Status::Success);
        space();
        ui::Progress("Fortschritt",d.progress,ui::Status::Success);
        space(5);
        ui::Options opt;opt.variant=ui::Variant::Outline;opt.fullWidth=narrow;
        if(ui::Button("Dialog öffnen",opt))d.modal=true;
    }
    ui::EndCard();
}
void sample(bool narrow){
    if(ui::BeginCard("##example")){
        introduction("Beispiel","So wirken die Bausteine zusammen.");
        ui::Muted("PROJEKT");
        ui::Heading("Website Relaunch");
        space(5);
        ui::Muted("Ein modernes Erscheinungsbild für alle Geräte.");
        space();
        ui::Progress("Umsetzung",.72f,ui::Status::Success);
        space();
        ui::Badge("In Arbeit",ui::Status::Info);
        space();
        ui::Options opt;opt.fullWidth=narrow;
        if(ui::Button("Details ansehen",opt))d.modal=true;
    }
    ui::EndCard();
}
void virtual_list(){
    introduction("Einträge","Direkt scrollen. Auch mit sehr langen Listen.");
    if(ui::BeginCard("##list-card")){
        ui::Muted("100.000 Einträge. Nur sichtbare Zeilen werden aufgebaut.");
        space(3);
        const float listHeight=std::max(220.f,ImGui::GetContentRegionAvail().y-22.f);
        ImGui::BeginChild("##virtual-list",{0,listHeight},ImGuiChildFlags_Borders);
        constexpr int count=100000;
        const float row=ui::control_height(ui::Size::MD)+4.f;
        ImGuiListClipper clipper;
        clipper.Begin(count,row);
        while(clipper.Step()){
            for(int i=clipper.DisplayStart;i<clipper.DisplayEnd;++i){
                char label[80];
                std::snprintf(label,sizeof(label),"Eintrag %06d",i+1);
                ImGui::PushID(i);
                if(ImGui::Selectable(label,d.listSelected==i,0,
                                     {0,row-ImGui::GetStyle().ItemSpacing.y}))
                    d.listSelected=i;
                ImGui::PopID();
                ++ui::scroll::state().rowsDrawn;
            }
        }
        // Inner list takes scroll first. Any remaining delta at the end of
        // the list can be passed to the parent window at its scroll edge.
        ui::scroll::apply();
        ImGui::EndChild();
    }
    ui::EndCard();
}
void theme(bool narrow){
    introduction("Erscheinungsbild","Eine gemeinsame Palette für jedes Element.");
    if(ui::BeginCard("##theme")){
        ui::Heading("Slate");
        ui::Muted("Hell und Dunkel mit abgestimmten Kontrasten.");
        space();
        if(ui::Switch("Dunkles Design",&d.dark))
            ui::set_mode(d.dark?ui::Mode::Dark:ui::Mode::Light);
        space(7);
        ui::Divider();space();
        ui::Muted("Farbrollen");
        ui::Badge("Info",ui::Status::Info);ImGui::SameLine();
        ui::Badge("Erfolg",ui::Status::Success);
        if(!narrow)ImGui::SameLine();else space();
        ui::Badge("Warnung",ui::Status::Warning);ImGui::SameLine();
        ui::Badge("Fehler",ui::Status::Error);
    }
    ui::EndCard();
}
}
void app::setup(){ui::initialize();ui::set_mode(ui::Mode::Light);}
void app::set_touch(bool value){
    if(value)ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_IsTouchScreen;
    else ImGui::GetIO().ConfigFlags&=~ImGuiConfigFlags_IsTouchScreen;
    ui::set_touch(value);
}
void app::safe_area(float a,float b,float c,float e){safeTop=a;safeRight=b;safeBottom=c;safeLeft=e;}
void app::keyboard_height(float h){keyboardHeight=std::max(0.f,h);}
void app::request_text_focus(){focusInput=true;d.page=1;}
void app::zoom(float ratio){ui::zoom(ratio);}
void app::draw(){
    ui::begin_frame();
    ui::scroll::begin_frame();
    auto& io=ImGui::GetIO();
    if(io.DisplaySize.x<=0||io.DisplaySize.y<=0)return;
    const bool narrow=io.DisplaySize.x<760.f;
    const float width=std::min(io.DisplaySize.x,1080.f);
    const float x=(io.DisplaySize.x-width)*.5f;
    const float top=safeTop;
    const float height=std::max(175.f,io.DisplaySize.y-top-safeBottom-keyboardHeight);
    ImGui::SetNextWindowPos({x,top},ImGuiCond_Always);
    ImGui::SetNextWindowSize({width,height},ImGuiCond_Always);
    ImGui::Begin("##component-gallery",nullptr,ImGuiWindowFlags_NoTitleBar|
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings);
    ui::Title("Komponenten");
    ui::Muted("Einfach. Einheitlich. Für alle Bildschirme.");
    space(8);
    const char* tabs[]={"Buttons","Formulare","Design","Liste"};
    ui::Tabs("##gallery-nav",tabs,4,&d.page);
    space(9);
    if(d.page==0){
        introduction("Grundelemente","Alles, was eine klare Oberfläche braucht.");
        const bool columns=!narrow&&ImGui::GetContentRegionAvail().x>720.f;
        if(columns)ImGui::Columns(2,"##two",false);
        buttons(narrow);
        if(columns)ImGui::NextColumn();else space(10);
        sample(narrow);
        if(columns)ImGui::Columns(1);
        space(12);
        feedback(narrow);
    }else if(d.page==1){
        introduction("Eingaben","Einheitliche Felder mit echten Interaktionszuständen.");
        const bool columns=!narrow&&ImGui::GetContentRegionAvail().x>720.f;
        if(columns)ImGui::Columns(2,"##forms",false);
        fields(narrow);
        if(columns)ImGui::NextColumn();else space(10);
        choices();
        if(columns)ImGui::Columns(1);
    }else if(d.page==2)theme(narrow);
    else virtual_list();
#ifdef __EMSCRIPTEN__
    if(d.page!=1)browser_input_rect(0,0,0,0);
#endif
    if(d.modal){ui::OpenModal("Bestätigung");d.modal=false;}
    if(ui::BeginModal("Bestätigung")){
        ui::Heading("Alles erledigt");
        ui::Muted("Deine Eingaben wurden übernommen.");
        space();
        if(ui::Button("Schließen",ui::Variant::Primary))
            ImGui::CloseCurrentPopup();
        ui::EndModal();
    }
    ui::scroll::apply();
    ui::scroll::end_frame();
    ImGui::End();
}
