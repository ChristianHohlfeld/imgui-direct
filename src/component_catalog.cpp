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
bool resetArticleScroll=false;
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
    case 8:return "float value=.5f; ui::Number(\"Wert\", &value, .05f, 0.f, 1.f);";
    case 9:return "int count=2; ui::Integer(\"Anzahl\", &count);";
    case 10:return "float pos[3]={1,2,3}; ui::Vector3(\"Position\", pos);";
    case 11:return "float color[4]={.4f,.3f,.9f,1}; ui::ColorEdit(\"Material\", color);";
    case 12:return "float low=.2f, high=.8f; ui::Range(\"Distanz\", &low, &high, 0.f, 1.f);";
    case 13:return "ui::FileInput(\"Asset\", \"modell.glb\", {});";
    case 15:return "ui::NavItem items[]={{\"Szene\"},{\"Assets\"}};\\nui::Navigation(\"nav\", items, &selected);";
    case 16:return "const char* path[]={\"Projekt\",\"Szene\",\"Aktuell\"};\\nui::Breadcrumbs(\"path\", path, &clicked);";
    case 17:return "ui::Pagination(\"pages\", 22, 5, &page);";
    case 18:return "if(ui::Accordion(\"Transform\",0,&expanded)) { /* Inhalt */ }";
    case 19:return "ui::Collapse(\"Details\", &opened);";
    case 20:return "if(ui::BeginDropdown(\"Aktionen\")) { ui::MenuItem(\"Öffnen\"); ui::EndDropdown(); }";
    case 21:return "ui::MenuItem(\"Datei öffnen\", \"Ctrl+O\");";
    case 22:return "if(ui::BeginContextMenu(\"menu\")) { ui::MenuItem(\"Kopieren\"); ui::EndContextMenu(); }";
    case 23:return "if(ui::Button(\"Dialog öffnen\")) ui::OpenModal(\"Dialog\");\\nif(ui::BeginModal(\"Dialog\")) { /* Inhalt */ ui::EndModal(); }";
    case 25:return "ui::BeginFieldset(\"group\", \"Objekte\"); /* Felder */ ui::EndFieldset();";
    case 26:return "ui::TableColumn columns[]={{\"Objekt\"},{\"Typ\"}};\\nif(ui::BeginTable(\"table\",columns)) { /* Zeilen */ ui::EndTable(); }";
    case 27:return "if(ui::TreeNode(\"root\",\"Projekt\")) { /* Kinder */ ui::EndTreeNode(); }";
    case 28:return "ui::VirtualList(\"list\", 10000, 100.f, 24.f, renderRow);";
    case 29:return "if(ui::BeginSplit(\"split\",&fraction)) { /* links */ }\\nif(ui::NextSplit()) { /* rechts */ } ui::EndSplit();";
    case 33:return "ui::RadialProgress(\"radial\", .75f, ui::Size::SM);";
    case 35:return "ui::Skeleton(\"skeleton\", {180,22});";
    case 36:return "ui::Stat(\"Aktive Objekte\", \"1.024\", \"+12%\");";
    case 37:return "ui::StatusDot(ui::Status::Success, \"Online\");";
    case 38:return "const char* steps[]={\"Start\",\"Test\",\"Fertig\"};\\nui::Steps(\"steps\", steps, 1);";
    case 39:return "ui::TimelineItem(\"Bearbeitet\", \"Heute, 12:00\", ui::Status::Success);";
    case 40:return "ui::Avatar(\"CH\");";
    case 41:return "ui::Kbd(\"Ctrl+S\");";
    case 42:return "ui::Button(\"Zeiger hier\"); ui::Tooltip(\"Hinweis\");";
    case 43:return "ui::Label(\"Bezeichnung\", true);";
    case 44:return "ui::Validation(\"Gültiger Wert\", ui::ValidationState::Success);";
    case 45:return "ui::Divider(\"Abschnitt\");";
    case 46:return "ui::Heading(\"Überschrift\"); ui::Muted(\"Beschreibung\");";
    case 47:return "ui::ToastQueue queue;\\nqueue.push(\"Gespeichert\",ui::Status::Success,now,3.0);\\nui::Toasts(\"toast\", queue, now);";
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
    ImGui::PushFont(nullptr,19.f*s);
    ImGui::TextUnformatted("Components");
    ImGui::PopFont();
    ImGui::Dummy({0,7.f*s});
    ui::Input("##docs-search",search,sizeof(search),"Search components ...");
    ImGui::Dummy({0,11.f*s});
    ImGui::Separator();
    for(int group=0;group<groupCount;++group){
        bool any=false;
        for(int i=0;i<review::count();++i)
            if(group_of(i)==group&&matches_index(i)){any=true;break;}
        if(!any)continue;
        ImGui::Dummy({0,13.f*s});
        ImGui::TextDisabled("%s",groups[group]);
        ImGui::Dummy({0,8.f*s});
        for(int i=0;i<review::count();++i){
            if(group_of(i)!=group||!matches_index(i))continue;
            ImGui::PushID(i);
            const bool active=chosen==i;
            if(active){
                const auto& p=ui::palette();
                ImGui::PushStyleColor(ImGuiCol_Header,
                    {p.neutral.r,p.neutral.g,p.neutral.b,1.f});
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                    {p.neutral.r,p.neutral.g,p.neutral.b,1.f});
                ImGui::PushStyleColor(ImGuiCol_Text,
                    {p.neutralContent.r,p.neutralContent.g,p.neutralContent.b,1.f});
            }
            if(ImGui::Selectable(review::component(i).name,active,0,{0,36.f*s}))
                switch_to(i);
            if(active)ImGui::PopStyleColor(3);
#ifdef __EMSCRIPTEN__
            const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
            browser_qa_item(1200+i,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
            ImGui::PopID();
        }
        ImGui::Dummy({0,8.f*s});
        ImGui::Separator();
    }
    ImGui::PopID();
}
void draw_mobile_picker(){
    const float s=ui::scale();
    // Native touch-editable search is independent of the navigation drawer.
    ui::Input("##docs-search-mobile",search,sizeof(search),"Komponente suchen ...");
    ImGui::Dummy({0,5.f*s});
    ui::Options menu;menu.variant=ui::Variant::Outline;menu.fullWidth=true;
    char label[128];
    std::snprintf(label,sizeof(label),"Komponenten  /  %s",review::component(chosen).name);
    if(ui::Button(label,menu))ImGui::OpenPopup("##docs-mobile-drawer");
#ifdef __EMSCRIPTEN__
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    browser_qa_item(1300,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
    const auto viewport=ImGui::GetMainViewport();
    const float width=std::min(viewport->Size.x-16.f*s,380.f*s);
    ImGui::SetNextWindowPos({viewport->Pos.x+8.f*s,viewport->Pos.y+8.f*s},ImGuiCond_Appearing);
    ImGui::SetNextWindowSize({width,std::max(220.f*s,viewport->Size.y-16.f*s)},ImGuiCond_Appearing);
    if(ImGui::BeginPopup("##docs-mobile-drawer",ImGuiWindowFlags_NoSavedSettings)){
        // The sidebar is the exact same native component navigator on desktop
        // and mobile. Popups dismiss on outside tap; selection closes it.
        draw_nav();
        ui::scroll::apply();
        if(changedSelection)ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
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
            if(n>0){
                const float next=ImGui::CalcTextSize(labels[n]).x+22.f*s;
                const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
                if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+next<right)
                    ImGui::SameLine();
            }
            ui::Button(labels[n],variants[n]);
            ImGui::PopID();
        }
    }else if(i==30){
        const char* labels[]={"Neutral","Primary","Secondary","Info","Success","Warning","Error"};
        const ui::Tone tones[]={ui::Tone::Neutral,ui::Tone::Primary,ui::Tone::Secondary,
            ui::Tone::Info,ui::Tone::Success,ui::Tone::Warning,ui::Tone::Error};
        for(int n=0;n<7;++n){
            if(n){
                const float next=ImGui::CalcTextSize(labels[n]).x+22.f*s;
                const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
                if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+next<right)
                    ImGui::SameLine();
            }
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

enum class ExampleKind { Standard, Colors, Sizes, Responsive, Outline, Soft, Disabled, Loading, Variants, State };
struct ExampleDoc {const char* title; const char* summary; ExampleKind kind;};
void button_example(ExampleKind kind){
    const float s=ui::scale();
    const char* label="Button";
    if(kind==ExampleKind::Sizes){
        constexpr ui::Size sizes[]={ui::Size::XS,ui::Size::SM,ui::Size::MD,ui::Size::LG,ui::Size::XL};
        constexpr const char* labels[]={"Xsmall","Small","Medium","Large","Xlarge"};
        for(int n=0;n<5;++n){
            ImGui::PushID(n);
            if(n){
                const float next=ImGui::CalcTextSize(labels[n]).x+45.f*s;
                const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
                if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+next<right)
                    ImGui::SameLine();
            }
            ui::Options options;options.variant=ui::Variant::Neutral;options.size=sizes[n];
            ui::Button(labels[n],options);
            ImGui::PopID();
        }
    }else if(kind==ExampleKind::Responsive){
        ui::Options options;options.variant=ui::Variant::Neutral;
        const float width=ImGui::GetContentRegionAvail().x;
        options.size=width<360.f*s?ui::Size::SM:width<700.f*s?ui::Size::MD:ui::Size::LG;
        ui::Button("Responsive",options);
    }else if(kind==ExampleKind::Colors){
        constexpr const char* labels[]={"Neutral","Primary","Secondary","Accent","Info","Success","Warning","Error"};
        constexpr ui::Variant variants[]={ui::Variant::Neutral,ui::Variant::Primary,ui::Variant::Secondary,
            ui::Variant::Accent,ui::Variant::Info,ui::Variant::Success,ui::Variant::Warning,ui::Variant::Error};
        for(int n=0;n<8;++n){
            ImGui::PushID(n);
            if(n){
                const float next=ImGui::CalcTextSize(labels[n]).x+43.f*s;
                const float right=ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x;
                if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+next<right)
                    ImGui::SameLine();
            }
            ui::Button(labels[n],variants[n]);
            ImGui::PopID();
        }
    }else if(kind==ExampleKind::Outline||kind==ExampleKind::Soft){
        const ui::Variant style=kind==ExampleKind::Outline?ui::Variant::Outline:ui::Variant::Soft;
        ui::Button("Default",style);ImGui::SameLine();
        ui::Button("Another",style);ImGui::SameLine();
        ui::Button("Action",style);
    }else if(kind==ExampleKind::Disabled||kind==ExampleKind::Loading){
        ui::Options options;options.variant=ui::Variant::Primary;
        options.state.disabled=kind==ExampleKind::Disabled;
        options.state.loading=kind==ExampleKind::Loading;
        ui::Button(kind==ExampleKind::Disabled?"Disabled":"Loading",options);
    }else{
        ui::Button(label,ui::Variant::Neutral);ImGui::SameLine();
        ui::Button("Primary",ui::Variant::Primary);
    }
}
const char* code_for_example(int i,ExampleKind kind){
    if(i==0){
        switch(kind){
        case ExampleKind::Sizes:return "ui::Options opt;\nopt.size = ui::Size::XS;  // XS, SM, MD, LG, XL\nui::Button(\"Xsmall\", opt);";
        case ExampleKind::Responsive:return "ui::Options opt;\nopt.size = viewportWidth < 700 ? ui::Size::SM : ui::Size::LG;\nui::Button(\"Responsive\", opt);";
        case ExampleKind::Colors:return "ui::Button(\"Neutral\", ui::Variant::Neutral);\nui::Button(\"Primary\", ui::Variant::Primary);\nui::Button(\"Success\", ui::Variant::Success);";
        case ExampleKind::Outline:return "ui::Button(\"Default\", ui::Variant::Outline);";
        case ExampleKind::Soft:return "ui::Button(\"Default\", ui::Variant::Soft);";
        case ExampleKind::Disabled:return "ui::Options opt;\nopt.state.disabled = true;\nui::Button(\"Disabled\", opt);";
        case ExampleKind::Loading:return "ui::Options opt;\nopt.state.loading = true;\nui::Button(\"Loading\", opt);";
        default:break;
        }
    }
    return code_example(i);
}
void preview_example(int i,ExampleKind kind){
    if(i==0){button_example(kind);return;}
    if(kind==ExampleKind::State){review::preview();return;}
    if(kind==ExampleKind::Variants){variant_sample(i);return;}
    if(kind==ExampleKind::Disabled){
        if(i==1){static char value[80]="Gesperrt";ui::Field f;f.disabled=true;ui::Input("Name",value,sizeof(value),"",f);return;}
        if(i==4){static bool value=true;ui::Field f;f.disabled=true;ui::Checkbox("Gesperrt",&value,f);return;}
        if(i==6){static bool value=true;ui::Field f;f.disabled=true;ui::Switch("Gesperrt",&value,f);return;}
    }
    sample(i);
}
void example_block(const ExampleDoc& doc,int i){
    const float s=ui::scale();
    ImGui::PushID(doc.title);
    ImGui::Dummy({0,24.f*s});
    ImGui::PushFont(nullptr,19.f*s);
    ImGui::TextUnformatted(doc.title);
    ImGui::PopFont();
    if(doc.summary&&*doc.summary){
        ImGui::Dummy({0,5.f*s});
        ui::Muted(doc.summary);
    }
    ImGui::Dummy({0,12.f*s});
    const auto& p=ui::palette();
    // Each example has one working Preview/C++ pair, exactly above its own
    // canvas. Unlike the old generic docs, the code tab belongs to this example.
    const ImGuiID tabKey=ImGui::GetID("##tab");
    ImGuiStorage* state=ImGui::GetStateStorage();
    const int current=state->GetInt(tabKey,0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{0,0});
    ImGui::PushStyleColor(ImGuiCol_Header,
        {p.base100.r,p.base100.g,p.base100.b,1.f});
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
        {p.base200.r,p.base200.g,p.base200.b,1.f});
    if(ImGui::Selectable("Preview##tab",current==0,0,{100.f*s,34.f*s}))state->SetInt(tabKey,0);
#ifdef __EMSCRIPTEN__
    if(i==0&&doc.kind==ExampleKind::Sizes){
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        browser_qa_item(1701,a.x,a.y,b.x-a.x,b.y-a.y);
    }
#endif
    ImGui::SameLine(0,0);
    if(ImGui::Selectable("C++##tab",current==1,0,{74.f*s,34.f*s}))state->SetInt(tabKey,1);
#ifdef __EMSCRIPTEN__
    if(i==0&&doc.kind==ExampleKind::Sizes){
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        browser_qa_item(1700,a.x,a.y,b.x-a.x,b.y-a.y);
    }
#endif
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    const int mode=state->GetInt(tabKey,0);
    const float height=(doc.kind==ExampleKind::Colors?195.f:
                        doc.kind==ExampleKind::State?230.f:154.f)*s;
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,p.metrics.radiusBox*s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{22.f*s,18.f*s});
    ImGui::PushStyleColor(ImGuiCol_ChildBg,{p.base100.r,p.base100.g,p.base100.b,1});
    ImGui::PushStyleColor(ImGuiCol_Border,{p.base300.r,p.base300.g,p.base300.b,1});
    if(ImGui::BeginChild("##example-stage",{0,height},ImGuiChildFlags_Borders,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        if(mode==1){
            const char* code=code_for_example(i,doc.kind);
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(code?code:"// See the public C++ component API in ui/components.hpp");
            ImGui::PopTextWrapPos();
        }else{
            // Very subtle diagonal reference-preview hatch drawn behind the
            // real native controls. No CSS, DOM, texture or overlay intercepts.
            const auto lo=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
            auto* draw=ImGui::GetWindowDrawList();
            const float h=size.y;
            const ImU32 stripe=ImGui::ColorConvertFloat4ToU32(
                {p.baseContent.r,p.baseContent.g,p.baseContent.b,.045f});
            draw->PushClipRect({lo.x+1,lo.y+1},{lo.x+size.x-1,lo.y+h-1},true);
            for(float x=lo.x-h;x<lo.x+size.x;x+=20.f*s)
                draw->AddLine({x,lo.y+h},{x+h,lo.y},stripe,1.f*s);
            draw->PopClipRect();
            const float w=ImGui::GetContentRegionAvail().x;
            const float liveWidth=std::min(w,900.f*s);
            const float spare=w-liveWidth;
            if(spare>0)ImGui::SetCursorPosX(ImGui::GetCursorPosX()+spare*.5f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg,{p.base100.r,p.base100.g,p.base100.b,0});
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
            if(ImGui::BeginChild("##live",{liveWidth,0},ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
                const bool small=doc.kind==ExampleKind::Standard||doc.kind==ExampleKind::Disabled||
                    doc.kind==ExampleKind::Outline||doc.kind==ExampleKind::Soft||
                    doc.kind==ExampleKind::Responsive||doc.kind==ExampleKind::Loading||
                    doc.kind==ExampleKind::Sizes;
                const float contentHeight=small?58.f*s:92.f*s;
                const float free=ImGui::GetContentRegionAvail().y-contentHeight;
                if(free>0)ImGui::SetCursorPosY(ImGui::GetCursorPosY()+free*.5f);
                // Do not center the first button only: center the full group.
                if(i==0){
                    float estimate=0;
                    switch(doc.kind){
                    case ExampleKind::Sizes:estimate=470.f*s;break;
                    case ExampleKind::Colors:estimate=750.f*s;break;
                    case ExampleKind::Responsive:estimate=150.f*s;break;
                    case ExampleKind::Outline:case ExampleKind::Soft:estimate=300.f*s;break;
                    case ExampleKind::Disabled:case ExampleKind::Loading:estimate=130.f*s;break;
                    default:estimate=185.f*s;
                    }
                    const float spareX=ImGui::GetContentRegionAvail().x-estimate;
                    if(spareX>0)ImGui::SetCursorPosX(ImGui::GetCursorPosX()+spareX*.5f);
                }
                preview_example(i,doc.kind);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);ImGui::PopStyleVar(2);
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
    if(chosen==0){
        const ExampleDoc examples[]={
            {"Button sizes","Fünf native Größen: von Xsmall bis Xlarge.",ExampleKind::Sizes},
            {"Responsive button","Die Größe passt sich an die verfügbare Breite an.",ExampleKind::Responsive},
            {"Button colors","Semantische Farben aus den gemeinsamen Theme-Tokens.",ExampleKind::Colors},
            {"Outline buttons","Zurückhaltende Aktionen mit Umrandung.",ExampleKind::Outline},
            {"Soft buttons","Dezente Schaltflächen für sekundäre Aktionen.",ExampleKind::Soft},
            {"Disabled button","Nicht verfügbare Aktionen sind sichtbar deaktiviert.",ExampleKind::Disabled},
            {"Loading button","Ladezustand direkt im nativen Button.",ExampleKind::Loading},
        };
        for(const auto& example:examples)example_block(example,chosen);
    }else{
        example_block({"Basic example","Direkt ausprobieren: echte native C++-Komponente.",ExampleKind::Standard},chosen);
        if(chosen==1||chosen==4||chosen==6||chosen==30||chosen==31||chosen==32||chosen==49)
            example_block({"Variants","Farben, Größen und verfügbare Varianten.",ExampleKind::Variants},chosen);
        if(chosen==1||chosen==4||chosen==6)
            example_block({"Disabled","Nicht verfügbar, ohne die Darstellung zu verstecken.",ExampleKind::Disabled},chosen);
    }
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
    example_block({"State preview","Echter ImGui-Zustand, keine Bildsimulation.",ExampleKind::State},chosen);
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
void reset_view_scroll(){resetArticleScroll=true;}
bool draw_index(int* outIndex){
    if(!outIndex)return false;
    changedSelection=false;
    inspectRequested=false;
    const float s=ui::scale();
    const bool desktop=ImGui::GetContentRegionAvail().x>=680.f*s;
    const bool resetScroll=resetArticleScroll;
    resetArticleScroll=false;
    ImGui::PushID("component-documentation");
    if(desktop){
        const float available=ImGui::GetContentRegionAvail().x;
        const float navWidth=std::clamp(available*.255f,235.f*s,370.f*s);
        ImGui::PushStyleColor(ImGuiCol_ChildBg,
            {ui::palette().base100.r,ui::palette().base100.g,ui::palette().base100.b,1});
        ImGui::BeginChild("##docs-sidebar",{navWidth,0},ImGuiChildFlags_None);
        draw_nav();
        ui::scroll::apply();
        ImGui::EndChild();ImGui::PopStyleColor();
        ImGui::SameLine(0,24.f*s);
        if(changedSelection||resetScroll)ImGui::SetNextWindowScroll({0,0});
        ImGui::BeginChild("##docs-article",{0,0},ImGuiChildFlags_None);
        draw_details();
        ui::scroll::apply();
        ImGui::EndChild();
    }else{
        draw_mobile_picker();
        ImGui::Dummy({0,7.f*s});
        if(changedSelection||resetScroll)ImGui::SetScrollY(0);
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
