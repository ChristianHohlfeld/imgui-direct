#include "app.hpp"
#include "ui/components.hpp"
#include "ui/theme.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#ifdef __EMSCRIPTEN__
extern "C" void browser_input_rect(float,float,float,float);
#endif

namespace {
struct Sample {
    int page=0, tier=1, activeTab=0, count=3;
    bool dark=false, notifications=true, marketing=false, autoSave=true;
    bool loadingDemo=false, agreed=true, showNotice=true, invalid=false, showModal=false;
    float progress=.64f, volume=.55f;
    char name[128]="Alex Morgan";
    char email[128]="alex@example.com";
    char note[360]="A quiet place for your next idea.";
} state;
float topInset=0,rightInset=0,bottomInset=0,leftInset=0,keyboardInset=0;
bool touchRequestedFocus=false;

void gap(float h=8.f){ImGui::Dummy({0,h*ui::scale()});}
void header(const char* title,const char* description){
    ui::Heading(title);
    ui::Muted(description);
    gap(8);
}
void overview(bool small){
    header("Dein Arbeitsbereich","Alle wichtigen Dinge an einem Ort.");
    if(ui::BeginCard("##summary")){
        ui::Muted("DIESE WOCHE");
        ui::Heading("Alles im Blick");
        gap(3);
        ui::Progress("Fortschritt",state.progress,ui::Status::Success);
        gap(8);
        ui::Badge("Im Zeitplan",ui::Status::Success);
        ImGui::SameLine();ui::Badge("3 Aufgaben offen",ui::Status::Info);
        gap(8);
        if(ui::Button("Aufgaben ansehen",{ui::Variant::Primary,ui::Size::MD,{},small})){
            state.page=1;
        }
    }
    ui::EndCard();
    gap();
    if(ui::BeginCard("##quick")){
        ui::Heading("Schnellaktionen");
        ui::Muted("Einfach, klar und direkt.");
        gap();
        ui::Options primary;primary.fullWidth=small;
        if(ui::Button("Neuen Eintrag erstellen",primary))state.showModal=true;
        if(!small)ImGui::SameLine();
        ui::Options quiet;quiet.variant=ui::Variant::Outline;quiet.fullWidth=small;
        if(ui::Button("Einstellungen",quiet))state.page=2;
        gap();
        ui::Muted("Änderungen werden in dieser Demo lokal gehalten.");
    }
    ui::EndCard();
}
void controls(bool small){
    header("Bausteine","Konsistente Elemente für jede Anwendung.");
    const bool split=!small&&ImGui::GetContentRegionAvail().x>=750.f;
    if(split)ImGui::Columns(2,"##split",false);
    if(ui::BeginCard("##actions")){
        ui::Heading("Aktionen");
        ui::Muted("Klare Hierarchie und eindeutige Rückmeldung.");
        gap(4);
        ui::Options p;p.fullWidth=small;
        if(ui::Button("Speichern",p)){state.count++;state.showNotice=true;}
        if(!small)ImGui::SameLine();
        ui::Options secondary;secondary.variant=ui::Variant::Outline;secondary.fullWidth=small;
        if(ui::Button("Abbrechen",secondary))state.showNotice=false;
        gap(3);
        ui::Options ghost;ghost.variant=ui::Variant::Ghost;ghost.fullWidth=small;
        if(ui::Button("Mehr erfahren",ghost))state.showModal=true;
        if(!small)ImGui::SameLine();
        ui::Options danger;danger.variant=ui::Variant::Error;danger.fullWidth=small;
        if(ui::Button("Entfernen",danger))state.count=std::max(0,state.count-1);
        gap(4);
        ui::Options busy;busy.variant=ui::Variant::Secondary;
        busy.state.loading=true;busy.fullWidth=small;
        ui::Button("Wird verarbeitet",busy);
        if(!small)ImGui::SameLine();
        ui::Options disabled;disabled.state.disabled=true;disabled.fullWidth=small;
        ui::Button("Nicht verfügbar",disabled);
        gap(2);
        ui::Badge("Aktiv",ui::Status::Success);ImGui::SameLine();
        ui::Badge("Information",ui::Status::Info);
    }
    ui::EndCard();
    if(split)ImGui::NextColumn();else gap();
    if(ui::BeginCard("##fields")){
        ui::Heading("Formular");
        ui::Muted("Eingaben, Auswahl und Prüfung.");
        gap(2);
        if(touchRequestedFocus){ImGui::SetKeyboardFocusHere();touchRequestedFocus=false;}
        ui::Field required;required.required=true;required.invalid=state.invalid;
        required.help=state.invalid?"Bitte gib einen Namen ein.":nullptr;
        ui::Input("Name",state.name,sizeof(state.name),"Vollständiger Name",required);
#ifdef __EMSCRIPTEN__
        {ImVec2 a,b;ui::LastInputBounds(a,b);
         browser_input_rect(a.x,a.y,b.x-a.x,b.y-a.y);}
#endif
        const char* types[]={"Persönlich","Team","Unternehmen"};
        ui::Select("Bereich",types,3,&state.tier);
        gap();
        if(ui::Button("Prüfen",ui::Variant::Primary)){
            state.invalid=std::strlen(state.name)==0;
            if(!state.invalid)state.showNotice=true;
        }
    }
    ui::EndCard();
    if(split)ImGui::Columns(1);
    gap();
    if(ui::BeginCard("##choices")){
        ui::Heading("Auswahl & Einstellungen");
        gap(4);
        ui::Switch("Benachrichtigungen",&state.notifications);
        ui::Checkbox("Automatisch speichern",&state.autoSave);
        ui::Radio("Standard",0,&state.activeTab);
        ui::Radio("Erweitert",1,&state.activeTab);
        ui::Slider("Lautstärke",&state.volume,0.f,1.f,"%.0f");
    }
    ui::EndCard();
    gap();
    ui::Field notes;
    notes.help="Deine Notizen bleiben in dieser Demo auf dem Gerät.";
    ui::Textarea("Notizen",state.note,sizeof(state.note),notes,4);
    gap();
    if(state.showNotice)
        ui::Alert("Gespeichert","Deine Änderungen wurden übernommen.",ui::Status::Success);
}
void settings(bool compact){
    header("Erscheinungsbild","Ein ruhiges Design. Zwei Farbmodi.");
    if(ui::BeginCard("##appearance")){
        ui::Heading("Design");
        gap(3);
        if(ui::Switch("Dunkles Erscheinungsbild",&state.dark))
            ui::set_mode(state.dark?ui::Mode::Dark:ui::Mode::Light);
        gap();
        ui::Muted("Systematisch abgestimmte Farben, Kontraste und Abstände.");
        gap();
        ui::Badge("Information",ui::Status::Info);ImGui::SameLine();
        ui::Badge("Erfolg",ui::Status::Success);ImGui::SameLine();
        ui::Badge("Hinweis",ui::Status::Warning);
    }
    ui::EndCard();
    gap();
    if(ui::BeginCard("##privacy")){
        ui::Heading("Präferenzen");
        gap(5);
        ui::Switch("Mitteilungen erhalten",&state.notifications);
        ui::Checkbox("Produktneuigkeiten",&state.marketing);
    }
    ui::EndCard();
    if(!compact){
        gap();
        ui::Muted("Einheitliche Elemente auf Smartphone, Tablet und Desktop.");
    }
}
}
void app::setup(){
    ui::initialize();
    ui::set_mode(ui::Mode::Light);
}
void app::set_touch(bool enabled){
    if(enabled)ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_IsTouchScreen;
    else ImGui::GetIO().ConfigFlags&=~ImGuiConfigFlags_IsTouchScreen;
    ui::set_touch(enabled);
}
void app::safe_area(float a,float b,float c,float d){topInset=a;rightInset=b;bottomInset=c;leftInset=d;}
void app::keyboard_height(float value){keyboardInset=std::max(0.f,value);}
void app::request_text_focus(){touchRequestedFocus=true;state.page=1;}
void app::zoom(float ratio){ui::zoom(ratio);}
void app::draw(){
    ui::begin_frame();
    ImGuiIO& io=ImGui::GetIO();
    if(io.DisplaySize.x<=0||io.DisplaySize.y<=0)return;
    const bool small=io.DisplaySize.x<700.f || (ui::is_touch() && io.DisplaySize.x<920.f);
    const float margin=small?6.f:20.f;
    const float left=leftInset+margin, top=topInset+margin;
    const float width=std::max(240.f,io.DisplaySize.x-left-rightInset-margin);
    const float height=std::max(180.f,io.DisplaySize.y-top-bottomInset-keyboardInset-margin);
    ImGui::SetNextWindowPos({left,top},ImGuiCond_Always);
    ImGui::SetNextWindowSize({width,height},ImGuiCond_Always);
    ImGui::Begin("##atelier",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|
                ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|
                ImGuiWindowFlags_NoSavedSettings);
    ui::Heading("atelier.");
    ui::Muted("Ein Ort für deine Ideen.");
    gap(7);
    const char* tabsWide[]={"Übersicht","Elemente","Design"};
    const char* tabsSmall[]={"Start","Elemente","Design"};
    ui::Tabs("navigation",small?tabsSmall:tabsWide,3,&state.page);
    ui::Divider();
    gap(8);
    if(state.page==0) overview(small);
    else if(state.page==1) controls(small);
    else settings(small);
#ifdef __EMSCRIPTEN__
    if(state.page!=1)browser_input_rect(0,0,0,0);
#endif
    if(state.showModal) {
        ui::OpenModal("Neuer Eintrag");state.showModal=false;
    }
    if(ui::BeginModal("Neuer Eintrag")){
        ui::Heading("Neuer Eintrag");
        ui::Muted("Ein neuer Eintrag für deinen Arbeitsbereich.");
        gap();
        if(ui::Button("Fertig",ui::Variant::Primary)){++state.count;ImGui::CloseCurrentPopup();}
        ImGui::SameLine();
        if(ui::Button("Schließen",ui::Variant::Ghost))ImGui::CloseCurrentPopup();
        ui::EndModal();
    }
    ImGui::End();
}
