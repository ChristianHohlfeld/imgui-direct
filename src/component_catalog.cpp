#include "component_catalog.hpp"
#include "state_gallery.hpp"
#include "ui/components.hpp"
#include "ui/extended.hpp"
#include "ui/essentials.hpp"
#include "ui/theme.hpp"
#include "ui/scroll.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#ifdef __EMSCRIPTEN__
extern "C" void browser_qa_item(int,float,float,float,float);
extern "C" void browser_qa_gallery(int,int,int);
#endif

namespace gallery {
namespace {
// The catalogue follows daisyUI's information architecture, not arbitrary
// array ranges: native menu/dialog belong to Actions; labels belong to Inputs.
constexpr const char* groups[]={"Aktionen","Eingaben","Navigation","Darstellung","Feedback","Layout"};
constexpr int groupCount=sizeof(groups)/sizeof(groups[0]);
constexpr const char* descriptions[]={
 "Aktionen auslösen, mit klarer visueller Priorität.",           // Button
 "Text einzeilig eingeben und validieren.",                    // Input
 "Mehrzeiligen Text komfortabel bearbeiten.",                 // Textarea
 "Eine Option aus mehreren Möglichkeiten wählen.",            // Select
 "Eine unabhängige Einstellung ein- oder ausschalten.",       // Checkbox
 "Eine von mehreren Optionen auswählen.",                     // Radio
 "Zustände über einen Schalter steuern.",                     // Switch
 "Werte durch Ziehen präzise einstellen.",                    // Slider
 "Dezimalwerte eingeben und verändern.",                      // Number
 "Ganzzahlige Werte bearbeiten.",                             // Integer
 "X-, Y- und Z-Koordinaten bearbeiten.",                      // Vector3
 "Farben mit RGBA-Werten festlegen.",                         // ColorEdit
 "Einen Wertebereich über zwei Grenzen wählen.",              // Range
 "Dateien auswählen und anzeigen.",                           // FileInput
 "Inhalte über Registerkarten organisieren.",                 // Tabs
 "Zwischen Bereichen navigieren.",                            // Navigation
 "Die Hierarchie des aktuellen Pfads zeigen.",                // Breadcrumbs
 "Durch mehrseitige Inhalte blättern.",                       // Pagination
 "Abschnitte einzeln auf- und zuklappen.",                   // Accordion
 "Details auf Wunsch ein- und ausblenden.",                   // Collapse
 "Ein Menü direkt an einer Aktion öffnen.",                  // Dropdown
 "Befehle in Menüs zugänglich machen.",                      // MenuItem
 "Aktionen durch Rechtsklick anbieten.",                     // ContextMenu
 "Wichtige Entscheidungen fokussiert abfragen.",              // Modal
 "Zusammengehörige Inhalte strukturieren.",                   // Card
 "Formularfelder sinnvoll gruppieren.",                       // Fieldset
 "Strukturierte Daten in Spalten darstellen.",                // Table
 "Hierarchische Objekte auf- und zuklappen.",                // TreeNode
 "Sehr große Datenmengen performant darstellen.",           // VirtualList
 "Bereiche durch Ziehen in der Größe ändern.",               // Split
 "Status und Kategorien kompakt markieren.",                  // Badge
 "Wichtige Meldungen deutlich hervorheben.",                 // Alert
 "Den Fortschritt eines Vorgangs anzeigen.",                 // Progress
 "Fortschritt in kompakter Kreisform zeigen.",               // RadialProgress
 "Laufende Aktionen visuell kennzeichnen.",                  // Loading
 "Platzhalter während des Ladens zeigen.",                  // Skeleton
 "Kennzahlen mit Kontext präsentieren.",                     // Stat
 "Verfügbarkeit durch Statuspunkte vermitteln.",             // StatusDot
 "Schritte eines Ablaufs sichtbar machen.",                   // Steps
 "Ereignisse chronologisch darstellen.",                     // TimelineItem
 "Personen oder Teams kompakt darstellen.",                   // Avatar
 "Tastenkombinationen leicht erfassbar machen.",              // Kbd
 "Zusatzinformationen beim Zeigen einblenden.",               // Tooltip
 "Felder klar und verständlich beschriften.",                // Label
 "Eingaben unmittelbar rückmelden.",                         // Validation
 "Abschnitte visuell voneinander trennen.",                  // Divider
 "Lesbare Überschriften und Text-Hierarchien.",              // Typography
 "Kurze, unaufdringliche Benachrichtigungen.",               // Toast
 "Links als erkennbare Navigation darstellen.",              // Link
 "Eine Bewertung mit Sternen vergeben.",                     // Rating
 "Zusammengehörige Auswahlknöpfe gruppieren.",               // Segmented
 "Nachrichten in Sprechblasen anordnen."                      // ChatBubble
};
static_assert(sizeof(descriptions)/sizeof(descriptions[0])==52);
char search[128]="";
int filter=0;
int drawnTiles=0;
int group_of(int i){
    if(i==0||i==20||i==21||i==22||i==23)return 0;
    if((i>=1&&i<=13)||i==43||i==44||i==49)return 1;
    if((i>=14&&i<=19)||i==38||i==48)return 2;
    if(i==24||i==26||i==27||i==28||i==30||i==36||i==37||
       i==39||i==40||i==41||i==51)return 3;
    if(i==31||i==32||i==33||i==34||i==35||i==42||i==47)return 4;
    return 5;
}
bool matches(const char* name,const char* term){
    if(!*term)return true;
    char buffer[192],needle[128];int j=0;
    for(const char* p=name;*p&&j<191;++p)buffer[j++]=(char)std::tolower((unsigned char)*p);
    buffer[j]=0;j=0;
    for(const char* p=term;*p&&j<127;++p)needle[j++]=(char)std::tolower((unsigned char)*p);
    needle[j]=0;return std::strstr(buffer,needle)!=nullptr;
}
ui::ToastQueue demoToasts;
void sample_list_row(int i,void*){
    ImGui::Text("Eintrag %04d",i+1);
}
void sample(int i){
    static char buffer[80]="Projektname";static char note[100]="Eine Beschreibung";
    static int selected=0,number=2,stars=4,expanded=-1,nav=0,page=0;
    static bool flag=true,opened=false;
    static float volume=.65f,vec[3]={1,2,3},color[4]={.4f,.3f,.9f,1},lo=.2f,hi=.8f,split=.46f;
    const char* options[]={"Szene","Assets","Einstellungen"};
    switch(i){
    case 0:ui::Button("Primär",ui::Variant::Primary);ImGui::SameLine();ui::Button("Soft",ui::Variant::Soft);break;
    case 1:ui::Input("Name",buffer,sizeof(buffer),"Name");break;
    case 2:ui::Textarea("Notiz",note,sizeof(note),{},2);break;
    case 3:ui::Select("Bereich",options,3,&selected);break;
    case 4:ui::Checkbox("Aktiv",&flag);break;
    case 5:ui::Radio("Szene",0,&selected);break;
    case 6:ui::Switch("Sichtbarkeit",&flag);break;
    case 7:ui::Slider("Deckkraft",&volume,0,1);break;
    case 8:ui::Number("Wert",&volume,.05f,0,1);break;
    case 9:ui::Integer("Anzahl",&number);break;
    case 10:ui::Vector3("Position",vec);break;
    case 11:ui::ColorEdit("Material",color);break;
    case 12:ui::Range("Distanz",&lo,&hi,0,1);break;
    case 13:ui::FileInput("Asset","modell.glb",{});break;
    case 14:ui::Tabs("tabs",options,3,&selected,ui::Size::XS);break;
    case 15:{ui::NavItem links[]={{"Szene"},{"Assets"}};ui::Navigation("nav",links,&nav,false,ui::Size::SM);break;}
    case 16:{const char* path[]={"Projekt","Szene","Aktuell"};int clicked=-1;ui::Breadcrumbs("path",path,&clicked);break;}
    case 17:ui::Pagination("pages",22,5,&page);break;
    case 18:if(ui::Accordion("Transform",0,&expanded))ui::Muted("Position · Rotation");break;
    case 19:ui::Collapse("Aufklappbare Details",&opened);break;
    case 20:if(ui::BeginDropdown("Aktionen")){ui::MenuItem("Öffnen");ui::EndDropdown();}break;
    case 21:ui::MenuItem("Datei öffnen","Ctrl+O");break;
    case 22:ui::Button("Rechtsklick",ui::Variant::Outline);if(ui::BeginContextMenu("menu")){ui::MenuItem("Kopieren");ui::EndContextMenu();}break;
    case 23:if(ui::Button("Dialog öffnen"))ui::OpenModal("Demo-Dialog");if(ui::BeginModal("Demo-Dialog")){ui::Muted("Eine modale Aktion.");if(ui::Button("Schließen"))ImGui::CloseCurrentPopup();ui::EndModal();}break;
    case 24:
        if(ui::BeginCard("##catalog-card",ui::CardOptions{ui::Size::XS,true})){
            ImGui::TextUnformatted("Projektkarte");
            ui::Badge("Bereit",{ui::Tone::Success,ui::Surface::Soft});
        }
        ui::EndCard();break;
    case 25:ui::BeginFieldset("fieldset","Objekte");ui::Muted("Einstellungen");ui::EndFieldset();break;
    case 26:{
        const ui::TableColumn columns[]={{"Objekt"},{"Typ"}};
        if(ui::BeginTable("##preview-table",columns,{.height=95.f*ui::scale()})){
            ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::TextUnformatted("Kamera");
            ImGui::TableNextColumn();ImGui::TextUnformatted("Scene");
            ui::EndTable();
        }
        break;
    }
    case 27:if(ui::TreeNode("root","Projekt")){ui::TreeNode("leaf","Kamera",false,true);ui::EndTreeNode();}break;
    case 28:{
        // The same real virtual-list primitive as the application uses.
        const float row=ImGui::GetTextLineHeightWithSpacing();
        ui::VirtualList("##catalog-virtual-list",10000,102.f*ui::scale(),row,sample_list_row);
        break;
    }
    case 29:{
        // The actual draggable splitter, not a static progress-bar mock.
        if(ui::BeginSplit("##catalog-split",&split,{0,105.f*ui::scale()},false,44.f*ui::scale()))
            ImGui::TextUnformatted("Links");
        if(ui::NextSplit())ImGui::TextUnformatted("Rechts");
        ui::EndSplit();
        break;
    }
    case 30:ui::Badge("Info",{ui::Tone::Info,ui::Surface::Soft});ImGui::SameLine();ui::Badge("Neu",{ui::Tone::Primary,ui::Surface::Solid});break;
    case 31:ui::Alert("Gespeichert","Änderung übernommen",{ui::Tone::Success,ui::Surface::Soft});break;
    case 32:ui::Progress("Fortschritt",.72f);break;
    case 33:ui::RadialProgress("radial",.75f,ui::Size::SM);break;
    case 34:ui::Loading("loading",ui::Size::SM);break;
    case 35:ui::Skeleton("skeleton",{180,22});ui::Skeleton("skeleton2",{110,14});break;
    case 36:ui::Stat("Aktive Objekte","1.024","+12% seit gestern");break;
    case 37:ui::StatusDot(ui::Status::Success,"Online");break;
    case 38:{const char* steps[]={"Start","Test","Fertig"};ui::Steps("steps",steps,1);break;}
    case 39:ui::TimelineItem("Bearbeitet","Heute, 12:00",ui::Status::Success);break;
    case 40:ui::Avatar("CH");ImGui::SameLine();ui::Avatar("AI");break;
    case 41:ui::Kbd("Ctrl+S");break;
    case 42:ui::Button("Zeiger hier");ui::Tooltip("Zusätzliche Informationen");break;
    case 43:ui::Label("Bezeichnung",true);break;
    case 44:ui::Validation("Gültiger Wert",ui::ValidationState::Success);break;
    case 45:ui::Divider("Abschnitt");break;
    case 46:ui::Heading("Überschrift");ui::Muted("Klar, lesbar, übersichtlich.");break;
    case 47:
        if(ui::Button("Toast anzeigen",ui::Variant::Primary))
            demoToasts.push("Änderung gespeichert",ui::Status::Success,ImGui::GetTime(),3.0);
        break;
    case 48:ui::Link("Details öffnen");break;
    case 49:ui::Rating("Sterne",&stars,5,ui::Size::SM);break;
    case 50:{const char* modes[]={"A","B","C"};ui::Segmented("join",modes,&selected);break;}
    case 51:ui::ChatBubble("Hallo, Welt!",true,ui::Tone::Primary);break;
    }
}


int chosen=0;
bool changedSelection=false;
bool inspectRequested=false;
const char* code_example(int i){
    switch(i){
    case 0:return "ui::Button(\"Speichern\", ui::Variant::Primary);";
    case 1:return "ui::Input(\"Name\", buffer, sizeof(buffer), \"Name eingeben\");";
    case 2:return "ui::Textarea(\"Notiz\", buffer, sizeof(buffer), {}, 3);";
    case 3:return "ui::Select(\"Bereich\", options, 3, &selected);";
    case 4:return "ui::Checkbox(\"Aktiv\", &enabled);";
    case 5:return "ui::Radio(\"Option\", 0, &selected);";
    case 6:return "ui::Switch(\"Aktiv\", &enabled);";
    case 7:return "ui::Slider(\"Deckkraft\", &value, 0.f, 1.f);";
    case 14:return "ui::Tabs(\"tabs\", items, 3, &selected);";
    case 24:return "if(ui::BeginCard(\"card\")) { /* Inhalt */ }\nui::EndCard();";
    case 30:return "ui::Badge(\"Bereit\", {ui::Tone::Success, ui::Surface::Soft});";
    case 31:return "ui::Alert(\"Erfolg\", \"Gespeichert\", {ui::Tone::Success, ui::Surface::Soft});";
    case 32:return "ui::Progress(\"Fortschritt\", .75f);";
    case 34:return "ui::Loading(\"loading\", ui::Size::MD);";
    case 48:return "if (ui::Link(\"Mehr erfahren\")) { /* Aktion */ }";
    case 49:return "ui::Rating(\"Bewertung\", &stars, 5, ui::Size::MD);";
    case 50:return "ui::Segmented(\"Ansicht\", labels, &selected);";
    case 51:return "ui::ChatBubble(\"Hallo!\", true, ui::Tone::Primary);";
    default:return nullptr;
    }
}
int matches_count(){
    int found=0;
    for(int i=0;i<review::count();++i)
        if(matches(review::component(i).name,search)||
           matches(descriptions[i],search))++found;
    return found;
}
bool matches_index(int i){
    return matches(review::component(i).name,search)||
           matches(descriptions[i],search);
}
void switch_to(int i){
    if(i==chosen)return;
    chosen=i;
    changedSelection=true;
    review::select(chosen,review::State::Normal);
}
void draw_nav(){
    const float s=ui::scale();
    ImGui::PushID("docs-navigation");
    ui::Input("##docs-search",search,sizeof(search),"Komponente suchen ...");
    ImGui::Dummy({0,9.f*s});
    for(int group=0;group<groupCount;++group){
        bool any=false;
        for(int i=0;i<review::count();++i)
            if(group_of(i)==group&&matches_index(i)){any=true;break;}
        if(!any)continue;
        ImGui::Dummy({0,6.f*s});
        ImGui::TextDisabled("%s",groups[group]);
        ImGui::Dummy({0,3.f*s});
        for(int i=0;i<review::count();++i){
            if(group_of(i)!=group||!matches_index(i))continue;
            ImGui::PushID(i);
            const bool active=chosen==i;
            if(active){
                ImGui::PushStyleColor(ImGuiCol_Header,
                    ImGui::GetColorU32({ui::palette().primary.r,ui::palette().primary.g,
                                       ui::palette().primary.b,.13f}));
                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::GetColorU32({ui::palette().primary.r,ui::palette().primary.g,
                                       ui::palette().primary.b,1.f}));
            }
            if(ImGui::Selectable(review::component(i).name,active,0,{0,30.f*s}))
                switch_to(i);
            if(active)ImGui::PopStyleColor(2);
#ifdef __EMSCRIPTEN__
            const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
            browser_qa_item(1200+i,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
            ImGui::PopID();
        }
    }
    ImGui::PopID();
}
void draw_mobile_picker(){
    const float s=ui::scale();
    ui::Input("##docs-search-mobile",search,sizeof(search),"Komponente suchen ...");
    ImGui::Dummy({0,4.f*s});
    ImGui::SetNextItemWidth(-1);
    if(ImGui::BeginCombo("##component-picker",review::component(chosen).name)){
        for(int group=0;group<groupCount;++group){
            bool any=false;
            for(int i=0;i<review::count();++i)
                if(group_of(i)==group&&matches_index(i)){any=true;break;}
            if(!any)continue;
            ImGui::SeparatorText(groups[group]);
            for(int i=0;i<review::count();++i){
                if(group_of(i)!=group||!matches_index(i))continue;
                if(ImGui::Selectable(review::component(i).name,chosen==i))
                    switch_to(i);
            }
        }
        ImGui::EndCombo();
    }
#ifdef __EMSCRIPTEN__
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    browser_qa_item(1300,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
}
void variant_sample(int i){
    const float s=ui::scale();
    if(i==0){
        const char* labels[]={"Neutral","Primary","Secondary","Accent","Info","Success","Warning","Error"};
        const ui::Variant variants[]={ui::Variant::Neutral,ui::Variant::Primary,
            ui::Variant::Secondary,ui::Variant::Accent,ui::Variant::Info,
            ui::Variant::Success,ui::Variant::Warning,ui::Variant::Error};
        for(int n=0;n<8;++n){
            ImGui::PushID(n);
            if(n>0&&ImGui::GetCursorPosX()+100.f*s<ImGui::GetWindowContentRegionMax().x)
                ImGui::SameLine();
            ui::Button(labels[n],variants[n]);
            ImGui::PopID();
        }
    }else if(i==30){
        const char* labels[]={"Neutral","Primary","Secondary","Info","Success","Warning","Error"};
        const ui::Tone tones[]={ui::Tone::Neutral,ui::Tone::Primary,ui::Tone::Secondary,
            ui::Tone::Info,ui::Tone::Success,ui::Tone::Warning,ui::Tone::Error};
        for(int n=0;n<7;++n){
            if(n)ImGui::SameLine();
            ui::Badge(labels[n],{tones[n],ui::Surface::Soft});
        }
    }else if(i==31){
        ui::Alert("Information","Eine hilfreiche Rückmeldung.",{ui::Tone::Info,ui::Surface::Soft});
        ui::Alert("Erfolg","Die Änderung wurde gespeichert.",{ui::Tone::Success,ui::Surface::Soft});
    }else if(i==1){
        static char user[96]="Alex";
        ui::Input("Vorname",user,sizeof(user),"Name");
        ImGui::Dummy({0,5.f*s});
        ui::Field disabled;disabled.disabled=true;
        ui::Input("Gesperrt",user,sizeof(user),"",disabled);
    }else if(i==32){
        ui::Progress("25 Prozent",.25f);
        ImGui::Dummy({0,12.f*s});
        ui::Progress("75 Prozent",.75f);
    }else if(i==6||i==4){
        static bool yes=true,no=false;
        if(i==6){ui::Switch("Aktiv",&yes);ui::Switch("Inaktiv",&no);}
        else{ui::Checkbox("Aktiv",&yes);ui::Checkbox("Inaktiv",&no);}
    }else if(i==49){
        static int rating=3;
        ui::Rating("Bewertung",&rating,5,ui::Size::LG);
    }else{
        sample(i);
    }
}
bool has_variants(int i){
    return i==0||i==1||i==4||i==6||i==30||i==31||i==32||i==49;
}
void example_block(const char* title,const char* summary,int i,bool variants,bool stateExample){
    const float s=ui::scale();
    ImGui::PushID(title);
    ImGui::Dummy({0,14.f*s});
    ImGui::PushFont(nullptr,18.f*s);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    if(summary&&*summary){
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,
             ImGui::GetColorU32({ui::palette().baseContent.r,ui::palette().baseContent.g,
                                 ui::palette().baseContent.b,.65f}));
        ImGui::TextWrapped("%s",summary);
        ImGui::PopStyleColor();
    }
    ImGui::Dummy({0,6.f*s});
    const auto& p=ui::palette();
    ImGui::PushStyleColor(ImGuiCol_ChildBg,
        {p.base200.r,p.base200.g,p.base200.b,1.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,p.metrics.radiusBox*s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{20.f*s,24.f*s});
    const float height=(stateExample?220.f:(variants&&i==31?235.f:195.f))*s;
    if(ImGui::BeginChild("##example",{0,height},ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        const float width=ImGui::GetContentRegionAvail().x;
        const float wanted=stateExample?570.f:540.f;
        const float spare=width-std::min(width,wanted*s);
        if(spare>0)ImGui::SetCursorPosX(ImGui::GetCursorPosX()+spare*.5f);
        if(ImGui::BeginChild("##live",{std::min(width,wanted*s),0},
            ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
            if(stateExample)review::preview();
            else if(variants)variant_sample(i);
            else sample(i);
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);ImGui::PopStyleColor();
    ImGui::PopID();
}
void draw_details(){
    const float s=ui::scale();
    const auto& entry=review::component(chosen);
    ImGui::PushID(chosen);
    ImGui::TextDisabled("KOMPONENTEN  /  %s",groups[group_of(chosen)]);
    ImGui::Dummy({0,7.f*s});
    ImGui::PushFont(nullptr,32.f*s);
    ImGui::TextUnformatted(entry.name);
#ifdef __EMSCRIPTEN__
    {
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        browser_qa_item(1400+chosen,a.x,a.y,b.x-a.x,b.y-a.y);
    }
#endif
    ImGui::PopFont();
    ImGui::Dummy({0,6.f*s});
    ImGui::PushTextWrapPos();
    ImGui::TextUnformatted(descriptions[chosen]);
    ImGui::PopTextWrapPos();
    ImGui::Dummy({0,14.f*s});
    ImGui::Separator();
    example_block("Standard","Interaktives Beispiel der nativen C++-Komponente.",chosen,false,false);
    if(has_variants(chosen))
        example_block("Varianten","Weitere Farben, Zustände und Darstellungsformen.",chosen,true,false);
    ImGui::Dummy({0,12.f*s});
    ImGui::PushFont(nullptr,18.f*s);
    ImGui::TextUnformatted("Interaktive Zustände");
    ImGui::PopFont();
    ImGui::TextDisabled("Direkt ausprobieren: Größe, deaktiviert, aktiv, Fehler und weitere unterstützte Zustände.");
    ImGui::SetNextItemWidth(std::min(330.f*s,ImGui::GetContentRegionAvail().x));
    int next=static_cast<int>(review::selected_state());
    if(ImGui::BeginCombo("##state-select",review::state_name(review::selected_state()))){
        for(int j=0;j<static_cast<int>(review::State::Count);++j){
            const auto st=static_cast<review::State>(j);
            if(review::supports(chosen,st)&&ImGui::Selectable(review::state_name(st),next==j))
                next=j;
        }
        ImGui::EndCombo();
    }
    if(next!=static_cast<int>(review::selected_state()))
        review::select(chosen,static_cast<review::State>(next));
    example_block("Zustandsvorschau","Echter ImGui-Zustand, keine Bildsimulation.",chosen,false,true);
    const char* code=code_example(chosen);
    if(code){
        ImGui::Dummy({0,16.f*s});
        ImGui::PushFont(nullptr,18.f*s);
        ImGui::TextUnformatted("C++");
        ImGui::PopFont();
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
            {ui::palette().base300.r,ui::palette().base300.g,ui::palette().base300.b,1});
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,ui::palette().metrics.radiusBox*s);
        if(ImGui::BeginChild("##code",{0,88.f*s},ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
            ImGui::TextWrapped("%s",code);
        }
        ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopStyleColor();
    }
    ImGui::Dummy({0,20.f*s});
    ui::Options detail;detail.size=ui::Size::SM;detail.variant=ui::Variant::Outline;
    const bool inspect=ui::Button("Alle Zustände detailliert prüfen",detail);
#ifdef __EMSCRIPTEN__
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    browser_qa_item(900,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
    if(inspect)inspectRequested=true;
    ImGui::PopID();
}
} // namespace
bool draw_index(int* outIndex){
    if(!outIndex)return false;
    changedSelection=false;
    inspectRequested=false;
    const float s=ui::scale();
    const bool desktop=ImGui::GetContentRegionAvail().x>=680.f*s;
    ImGui::PushID("component-documentation");
    if(desktop){
        const float navWidth=235.f*s;
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
            {ui::palette().base100.r,ui::palette().base100.g,ui::palette().base100.b,1});
        ImGui::BeginChild("##docs-sidebar",{navWidth,0},ImGuiChildFlags_None);
        draw_nav();
        ui::scroll::apply();
        ImGui::EndChild();ImGui::PopStyleColor();
        ImGui::SameLine(0,18.f*s);
        if(changedSelection)ImGui::SetNextWindowScroll({0,0});
        ImGui::BeginChild("##docs-article",{0,0},ImGuiChildFlags_None);
        draw_details();
        ui::scroll::apply();
        ImGui::EndChild();
    }else{
        draw_mobile_picker();
        ImGui::Dummy({0,7.f*s});
        if(changedSelection)ImGui::SetScrollY(0);
        draw_details();
    }
#ifdef __EMSCRIPTEN__
    browser_qa_gallery(matches_count(),1,group_of(chosen)+1);
#endif
    ImGui::PopID();
    if(inspectRequested){*outIndex=chosen;return true;}
    return false;
}
} // namespace gallery
