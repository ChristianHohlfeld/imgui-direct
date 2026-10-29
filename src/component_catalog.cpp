#include "component_catalog.hpp"
#include "state_gallery.hpp"
#include "ui/components.hpp"
#include "ui/extended.hpp"
#include "ui/essentials.hpp"
#include "ui/theme.hpp"
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
void sample(int i){
    static char buffer[80]="Projektname";static char note[100]="Eine Beschreibung";
    static int selected=0,number=2,stars=4,expanded=-1,nav=0,page=0;
    static bool flag=true,opened=false;
    static float volume=.65f,vec[3]={1,2,3},color[4]={.4f,.3f,.9f,1},lo=.2f,hi=.8f;
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
    case 28:for(int n=0;n<3;++n)ImGui::BulletText("Element %d",n+1);break;
    case 29:ui::Muted("Outliner  |  Inspector");ui::Progress("Größe",.4f);break;
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
    case 47:ui::Badge("Benachrichtigung",{ui::Tone::Info,ui::Surface::Soft});break;
    case 48:ui::Link("Details öffnen");break;
    case 49:ui::Rating("Sterne",&stars,5,ui::Size::SM);break;
    case 50:{const char* modes[]={"A","B","C"};ui::Segmented("join",modes,&selected);break;}
    case 51:ui::ChatBubble("Hallo, Welt!",true,ui::Tone::Primary);break;
    }
}

constexpr float tileHeight=248.f;
constexpr float heroHeight=142.f;
float sample_height(int i){
    // Give multiline/native complex controls enough room to render entirely.
    if(i==2||i==10||i==11||i==24||i==25||i==26||i==29||i==36||i==46||i==51)
        return 120.f;
    if(i==1||i==3||i==7||i==8||i==9||i==12||i==13||i==31||i==39)
        return 95.f;
    return 62.f;
}
bool tile(int index){
    const auto& meta=review::component(index);
    const auto& pal=ui::palette();
    const float s=ui::scale();
    const ImVec4 previewSurface{pal.base200.r,pal.base200.g,pal.base200.b,1};
    ImGui::PushID(index);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{5.f*s,5.f*s});
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4{pal.base100.r,pal.base100.g,pal.base100.b,0.f});
    bool opened=false;
    if(ImGui::BeginChild("##component-tile",{0,tileHeight*s},ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        // As in daisyUI's actual component index: a spacious, independent
        // preview ABOVE the component title and explanatory text, not a
        // mostly empty boxed form with a tiny sample in its top-left corner.
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,pal.metrics.radiusBox*s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{8.f*s,6.f*s});
        ImGui::PushStyleColor(ImGuiCol_ChildBg,previewSurface);
        if(ImGui::BeginChild("##visual-preview",{0,heroHeight*s},ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
            const ImVec2 area=ImGui::GetContentRegionAvail();
            const float contentW=std::min(area.x,244.f*s);
            const float contentH=std::min(area.y,sample_height(index)*s);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,(area.y-contentH)*.5f));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.f,(area.x-contentW)*.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
            if(ImGui::BeginChild("##live-control",{contentW,contentH},ImGuiChildFlags_None,
                                ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse))
                sample(index);
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();ImGui::PopStyleVar(2);
        ImGui::Dummy({0,4.f*s});
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4{pal.baseContent.r,pal.baseContent.g,pal.baseContent.b,1.f});
        ImGui::TextUnformatted(meta.name);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("  %02d",index+1);
        ui::Muted(descriptions[index]);
        // Fixed baseline across all cards, regardless of description length.
        const float footerY=(tileHeight-ui::control_height(ui::Size::SM)/s-6.f)*s;
        if(ImGui::GetCursorPosY()<footerY)ImGui::SetCursorPosY(footerY);
        ui::Options action;action.variant=ui::Variant::Ghost;action.size=ui::Size::SM;
        opened=ui::Button("Details / Zustände",action);
#ifdef __EMSCRIPTEN__
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        browser_qa_item(900+index,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();ImGui::PopStyleVar(2);ImGui::PopID();
    ++drawnTiles;
    return opened;
}
void filter_buttons(bool vertical,float width){
    constexpr const char* options[]={"Alle","Aktionen","Eingaben","Navigation",
                                      "Darstellung","Feedback","Layout"};
    const float s=ui::scale(),gap=ImGui::GetStyle().ItemSpacing.x;
    float consumed=0.f;
    ImGui::PushID("catalog-filters");
    for(int i=0;i<groupCount+1;++i){
        const float w=vertical?width:
            std::min(width,ImGui::CalcTextSize(options[i]).x+27.f*s);
        if(!vertical&&i>0&&consumed+w+gap<=width){
            ImGui::SameLine();consumed+=gap;
        }else consumed=0;
        ImGui::PushID(i);
        ui::Options b;b.size=ui::Size::SM;
        b.variant=filter==i?ui::Variant::Soft:ui::Variant::Ghost;
        b.state.selected=filter==i;
        if(ui::Button(options[i],b,{w,0})){
            if(filter!=i){filter=i;ImGui::SetScrollY(0);}
        }
#ifdef __EMSCRIPTEN__
        const auto a=ImGui::GetItemRectMin(),bb=ImGui::GetItemRectMax();
        browser_qa_item(1000+i,a.x,a.y,bb.x-a.x,bb.y-a.y);
#endif
        ImGui::PopID();
        consumed+=w;
    }
    ImGui::PopID();
}
bool matches_item(int i){
    if(filter>0&&group_of(i)!=filter-1)return false;
    return matches(review::component(i).name,search)||
           matches(descriptions[i],search)||
           matches(groups[group_of(i)],search);
}
bool draw_groups(int* outIndex){
    const float s=ui::scale();
    const int count=review::count();
    int matching=0;for(int i=0;i<count;++i)if(matches_item(i))++matching;
    ImGui::TextDisabled("%d von %d Komponenten",matching,count);
    if(!matching){
        ImGui::Dummy({0,16.f*s});
        ui::Muted("Keine Treffer. Suche oder Kategorie ändern.");
#ifdef __EMSCRIPTEN__
        browser_qa_gallery(matching,0,filter);
#endif
        return false;
    }
    const float width=ImGui::GetContentRegionAvail().x;
    const int cols=width<470.f*s?1:width<800.f*s?2:3;
    const ImGuiTableFlags flags=ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_NoSavedSettings;
    const float pad=7.f*s;
    const float rowHeight=tileHeight*s+2.f*pad;
    drawnTiles=0;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,{6.f*s,pad});
    for(int group=0;group<groupCount;++group){
        if(filter>0&&group!=filter-1)continue;
        std::array<int,52> indices{};int groupSize=0;
        for(int i=0;i<count;++i)
            if(group_of(i)==group&&matches_item(i))indices[groupSize++]=i;
        if(!groupSize)continue;
        ImGui::SeparatorText(groups[group]);
        ImGui::PushID(group);
        if(ImGui::BeginTable("##catalog-grid",cols,flags)){
            // Virtualize rows, not columns: only on-screen card previews
            // build full ImGui draw lists even in the 52-component catalogue.
            ImGuiListClipper clipper;
            clipper.Begin((groupSize+cols-1)/cols,rowHeight);
            while(clipper.Step()){
                for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;++row){
                    ImGui::TableNextRow(ImGuiTableRowFlags_None,rowHeight);
                    for(int col=0;col<cols;++col){
                        const int n=row*cols+col;
                        if(n>=groupSize)break;
                        ImGui::TableSetColumnIndex(col);
                        if(tile(indices[n])){
                            *outIndex=indices[n];
                            clipper.End();ImGui::EndTable();ImGui::PopID();ImGui::PopStyleVar();
                            return true;
                        }
                    }
                }
            }
            clipper.End();
            ImGui::EndTable();
        }
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
#ifdef __EMSCRIPTEN__
    browser_qa_gallery(matching,drawnTiles,filter);
#endif
    return false;
}
} // namespace
bool draw_index(int* outIndex){
    if(!outIndex)return false;
    const float s=ui::scale(),width=ImGui::GetContentRegionAvail().x;
    ImGui::PushID("component-index");
    // Search is a native input; all 52 examples remain native C++ controls.
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{12.f*s,9.f*s});
    ImGui::SetNextItemWidth(-1);
    const bool modified=ImGui::InputTextWithHint("##search",
        "Komponenten durchsuchen (Name, Funktion, Kategorie)",search,sizeof(search));
    ImGui::PopStyleVar();
    if(modified)ImGui::SetScrollY(0);
    ImGui::Dummy({0,5.f*s});
    bool opened=false;
    if(width>=850.f*s){
        if(ImGui::BeginTable("##browser",2,
                 ImGuiTableFlags_SizingFixedFit|ImGuiTableFlags_NoSavedSettings)){
            ImGui::TableSetupColumn("Kategorien",ImGuiTableColumnFlags_WidthFixed,160.f*s);
            ImGui::TableSetupColumn("Vorschauen",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("KATEGORIEN");
            filter_buttons(true,std::max(1.f,ImGui::GetContentRegionAvail().x));
            ImGui::TableNextColumn();
            opened=draw_groups(outIndex);
            ImGui::EndTable();
        }
    }else{
        filter_buttons(false,width);
        ImGui::Dummy({0,8.f*s});
        opened=draw_groups(outIndex);
    }
    ImGui::PopID();
    return opened;
}
} // namespace gallery
