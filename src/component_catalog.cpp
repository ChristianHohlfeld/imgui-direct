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
#include <numeric>
#ifdef __EMSCRIPTEN__
extern "C" void browser_qa_item(int,float,float,float,float);
#endif

namespace gallery {
namespace {
constexpr const char* groups[]={"Aktionen","Eingaben","Navigation","Layout","Feedback"};
constexpr const char* descriptions[] = {
    "Aktionen auslösen und Varianten vergleichen.",
    "Text eingeben, ändern und validieren.",
    "Mehrzeilige Texte direkt bearbeiten.",
    "Eine Option aus mehreren wählen.",
    "Eine unabhängige Auswahl umschalten.",
    "Genau eine Auswahl treffen.",
    "Zustände direkt ein- oder ausschalten.",
    "Werte stufenlos einstellen.",
    "Dezimalwerte präzise bearbeiten.",
    "Ganze Zahlen mit Schritten anpassen.",
    "Räumliche X-, Y- und Z-Werte eingeben.",
    "Farben und Alphawerte bearbeiten.",
    "Einen zusammenhängenden Bereich wählen.",
    "Dateien für die Anwendung auswählen.",
    "Ansichten in Reitern wechseln.",
    "Zwischen Bereichen navigieren.",
    "Den aktuellen Pfad nachvollziehen.",
    "Durch mehrere Ergebnisseiten blättern.",
    "Inhalt gezielt ein- und ausklappen.",
    "Zusätzliche Informationen aufklappen.",
    "Kontextaktionen aufrufen.",
    "Befehle in einem Menü wählen.",
    "Aktionen zum Objekt anbieten.",
    "Dialoge mit Fokussteuerung öffnen.",
    "Informationen als Karte gruppieren.",
    "Einstellungen logisch zusammenfassen.",
    "Daten in Spalten übersichtlich zeigen.",
    "Hierarchien ein- und ausklappen.",
    "Elemente in einer Liste ordnen.",
    "Flächen flexibel aufteilen.",
    "Kurze Statusmarkierungen darstellen.",
    "Wichtige Rückmeldungen hervorheben.",
    "Fortschritt als Balken darstellen.",
    "Fortschritt im Kreis ablesen.",
    "Laufende Vorgänge animiert anzeigen.",
    "Platzhalter während des Ladens zeigen.",
    "Kennzahlen und Trends hervorheben.",
    "Einen Zustand auf einen Blick erkennen.",
    "Mehrstufige Abläufe visualisieren.",
    "Ereignisse zeitlich anordnen.",
    "Personen oder Objekte repräsentieren.",
    "Tastenkürzel klar kennzeichnen.",
    "Hinweise bei Bedarf einblenden.",
    "Bedienelemente eindeutig benennen.",
    "Fehler und Erfolge verständlich melden.",
    "Inhalte optisch voneinander trennen.",
    "Hierarchische Überschriften nutzen.",
    "Informationen dezent hervorheben.",
    "Navigation als echten Link anbieten.",
    "Bewertungen direkt mit Sternen wählen.",
    "Verbundene Optionen gemeinsam wählen.",
    "Nachrichten als Sprechblasen zeigen."
};
static_assert(std::size(descriptions)==52);

char search[128]="";
int activeGroup=-1;
int group_of(int i) {
    if(i==0||i==41||i==48||i==50)return 0;
    if((i>=1&&i<=13)||i==43||i==44||i==49)return 1;
    if((i>=14&&i<=23)||i==38)return 2;
    if(i>=24&&i<=29||i==45||i==46)return 3;
    return 4;
}
const char* group_name(int i){return groups[group_of(i)];}
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

constexpr float previewHeight=142.f;
constexpr float tileHeight=238.f;
bool is_short_preview(int i){
    switch(i){
    case 0:case 4:case 5:case 6:case 16:case 17:
    case 19:case 21:case 30:case 32:case 33:case 34:
    case 37:case 40:case 41:case 42:case 43:case 44:
    case 45:case 47:case 48:case 49:case 50:
        return true;
    default:return false;
    }
}
void mark_catalog_item(int id){
#ifdef __EMSCRIPTEN__
    const ImVec2 a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    browser_qa_item(id,a.x,a.y,b.x-a.x,b.y-a.y);
#else
    (void)id;
#endif
}
bool tile(int index){
    const auto& meta=review::component(index);
    const auto& pal=ui::palette();
    const float s=ui::scale();
    const float radius=std::max(8.f,pal.metrics.radiusBox)*s;
    // Exactly one hierarchy: preview surface -> title -> short explanation ->
    // direct route to the full interactive state inspector. No decorative
    // card-within-card frame, invisible overlays, or duplicate action footer.
    ImGui::PushID(index);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,radius);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::PushStyleColor(ImGuiCol_ChildBg,{pal.base100.r,pal.base100.g,pal.base100.b,1});
    bool opened=false;
    if(ImGui::BeginChild("##tile",{0,tileHeight*s},ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,radius);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12.f*s,10.f*s});
        ImGui::PushStyleColor(ImGuiCol_ChildBg,{pal.base200.r,pal.base200.g,pal.base200.b,1});
        if(ImGui::BeginChild("##preview",{0,previewHeight*s},ImGuiChildFlags_None,
                            ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
            if(is_short_preview(index)){
                const float available=previewHeight-20.f;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY()
                    +std::max(0.f,(available-ui::control_height(ui::Size::MD))*.5f));
            }
            sample(index);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();ImGui::PopStyleVar(2);
        ImGui::Dummy({0,4.f*s});
        ImGui::TextUnformatted(meta.name);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled,
            {pal.baseContent.r,pal.baseContent.g,pal.baseContent.b,.64f});
        ImGui::TextUnformatted(descriptions[index]);
        ImGui::PopStyleColor();
        opened=ui::Link("Zustände ansehen");
        mark_catalog_item(900+index);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();ImGui::PopStyleVar(2);ImGui::PopID();
    return opened;
}
int filter=0;
int group_count(int group){
    int count=0;
    for(int i=0;i<review::count();++i)
        if(group<0||group_of(i)==group)++count;
    return count;
}
void filter_buttons(bool vertical,float width){
    const char* labels[]={"Alle","Aktionen","Eingaben","Navigation","Layout","Feedback"};
    const float s=ui::scale();
    // Two compact rows on narrow screens; a genuine category sidebar on desktop.
    ImGui::PushID("catalog-filter");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{4.f*s,4.f*s});
    const float buttonWidth=vertical?width:std::max(1.f,(width-8.f*s)/3.f);
    for(int i=0;i<6;++i){
        if(!vertical&&i%3)ImGui::SameLine();
        ImGui::PushID(i);
        ui::Options b;
        b.size=ui::Size::SM;
        b.variant=filter==i?ui::Variant::Soft:ui::Variant::Ghost;
        b.state.selected=filter==i;
        if(ui::Button(labels[i],b,{buttonWidth,0}))filter=i;
        mark_catalog_item(940+i);
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
}
bool matches_component(int i){
    const auto& component=review::component(i);
    return (filter==0||group_of(i)==filter-1)
        &&(matches(component.name,search)||matches(descriptions[i],search));
}
bool draw_grid(int* outIndex){
    const int count=review::count();
    std::array<int,52> indices{};
    int shown=0;
    for(int i=0;i<count;++i)if(matches_component(i))indices[shown++]=i;
    std::sort(indices.begin(),indices.begin()+shown,[](int a,int b){
        return std::strcmp(review::component(a).name,review::component(b).name)<0;
    });
    const float s=ui::scale(),width=ImGui::GetContentRegionAvail().x;
    ImGui::TextDisabled("%d von %d Komponenten",shown,count);
    if(!shown){
        ImGui::Dummy({0,14.f*s});
        ui::Muted("Keine Treffer. Suche oder Kategorie ändern.");
        return false;
    }
    const int cols=width<510.f*s?1:width<780.f*s?2:3;
    const float padding=7.f*s;
    const ImGuiTableFlags flags=ImGuiTableFlags_SizingStretchSame
                              |ImGuiTableFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,{padding,padding});
    if(ImGui::BeginTable("##components-grid",cols,flags)){
        const int rows=(shown+cols-1)/cols;
        // ImGuiListClipper visits only on-screen rows: no 52 child windows and
        // expensive previews rebuilt every frame while scrolling the catalog.
        ImGuiListClipper clipper;
        clipper.Begin(rows,tileHeight*s+2.f*padding);
        while(clipper.Step()){
            for(int row=clipper.DisplayStart;row<clipper.DisplayEnd;++row){
                ImGui::TableNextRow(ImGuiTableRowFlags_None,tileHeight*s);
                for(int col=0;col<cols;++col){
                    ImGui::TableSetColumnIndex(col);
                    const int cell=row*cols+col;
                    if(cell>=shown)continue;
                    if(tile(indices[cell])){
                        *outIndex=indices[cell];
                        clipper.End();
                        ImGui::EndTable();ImGui::PopStyleVar();
                        return true;
                    }
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    return false;
}
} // namespace

bool draw_index(int* outIndex){
    if(!outIndex)return false;
    const float s=ui::scale(),width=ImGui::GetContentRegionAvail().x;
    ImGui::PushID("component-index");
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{8.f*s,7.f*s});
    // A single search control, without a second oversized form label.
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##catalog-search","Komponente suchen ...",search,sizeof(search));
    ImGui::Dummy({0,3.f*s});
    bool opened=false;
    if(width>=850.f*s){
        if(ImGui::BeginTable("##browser",2,
             ImGuiTableFlags_SizingFixedFit|ImGuiTableFlags_NoSavedSettings)){
            ImGui::TableSetupColumn("Kategorien",ImGuiTableColumnFlags_WidthFixed,160.f*s);
            ImGui::TableSetupColumn("Vorschau",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("KATEGORIEN");
            filter_buttons(true,std::max(1.f,ImGui::GetContentRegionAvail().x));
            ImGui::TableNextColumn();
            opened=draw_grid(outIndex);
            ImGui::EndTable();
        }
    }else{
        filter_buttons(false,width);
        ImGui::Dummy({0,4.f*s});
        opened=draw_grid(outIndex);
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return opened;
}
} // namespace gallery
