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

namespace gallery {
namespace {
constexpr const char* groups[]={"Aktionen","Eingaben","Navigation","Layout","Feedback"};
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
    case 24:ui::Muted("Inhalt einer Karte");ui::Badge("Bereit",{ui::Tone::Success,ui::Surface::Soft});break;
    case 25:ui::BeginFieldset("fieldset","Objekte");ui::Muted("Einstellungen");ui::EndFieldset();break;
    case 26:ImGui::TextUnformatted("Objekt    Typ");ImGui::Separator();ImGui::TextUnformatted("Kamera    Scene");break;
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
bool tile(int index){
    const auto& meta=review::component(index);
    const float radius=ui::palette().metrics.radiusBox*ui::scale();
    const float border=ui::palette().metrics.border;
    ImGui::PushID(index);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,radius);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{14.f*ui::scale(),12.f*ui::scale()});
    ImGui::PushStyleColor(ImGuiCol_ChildBg,{ui::palette().base100.r,ui::palette().base100.g,ui::palette().base100.b,1});
    ImGui::PushStyleColor(ImGuiCol_Border,{ui::palette().base300.r,ui::palette().base300.g,ui::palette().base300.b,1});
    bool opened=false;
    if(ImGui::BeginChild("##tile",{0,165.f*ui::scale()},ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        const auto& p=ui::palette();
        ImGui::PushStyleColor(ImGuiCol_Text,{p.baseContent.r,p.baseContent.g,p.baseContent.b,1});
        ImGui::TextUnformatted(meta.name);
        ImGui::PopStyleColor();
        ImGui::SameLine();ImGui::TextDisabled("  %02d",index+1);
        const ImVec2 titleBottom=ImGui::GetCursorScreenPos();
        // Visible, genuine live component sample. Its own inputs stay independent
        // of the select/open action in the card footer.
        ImGui::BeginChild("##preview",{0,85.f*ui::scale()},0,
            ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        sample(index);
        ImGui::EndChild();
        const float rowHeight=ui::control_height(ui::Size::SM);
        ui::Options select;select.variant=ui::Variant::Ghost;select.size=ui::Size::SM;select.fullWidth=true;
        opened=ui::Button("Ansehen / Zustände",select,{0,rowHeight});
        (void)titleBottom;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);ImGui::PopStyleVar(2);ImGui::PopID();
    return opened;
}
}
bool draw_index(int* outIndex){
    if(!outIndex)return false;
    const auto& p=ui::palette();
    ui::Heading("Alle Komponenten");
    ui::Muted("Direkt ansehen, ausprobieren und jeden Zustand prüfen – wie im daisyUI-Katalog.");
    ImGui::Dummy({0,7.f*ui::scale()});
    ui::Input("Suche",search,sizeof(search),"Komponente suchen …");
    const float width=ImGui::GetContentRegionAvail().x;
    const int tabs=6;
    static int filter=0;
    const char* names[]={"Alle","Aktionen","Eingaben","Navigation","Layout","Feedback"};
    const float pillWidth=std::max(65.f*ui::scale(),width/3.f);
    for(int i=0;i<tabs;++i){
        if(i%3)ImGui::SameLine();
        ImGui::PushID(i);
        ui::Options op;op.size=ui::Size::SM;op.variant=filter==i?ui::Variant::Primary:ui::Variant::Ghost;
        if(ui::Button(names[i],op,{pillWidth-6.f*ui::scale(),0}))filter=i;
        ImGui::PopID();
    }
    activeGroup=filter-1;
    const int count=review::count();
    const int cols=width<550.f*ui::scale()?1:width<850.f*ui::scale()?2:3;
    int matchesCount=0;
    for(int i=0;i<count;++i)if((activeGroup<0||activeGroup==group_of(i))&&matches(review::component(i).name,search))++matchesCount;
    ImGui::PushStyleColor(ImGuiCol_Text,{p.baseContent.r,p.baseContent.g,p.baseContent.b,.62f});
    ImGui::Text("%d von %d Komponenten",matchesCount,count);
    ImGui::PopStyleColor();
    if(matchesCount==0){ImGui::TextDisabled("Keine Treffer. Suche oder Filter ändern.");return false;}
    ImGuiTableFlags flags=ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_NoSavedSettings;
    for(int group=0;group<5;++group){
        if(activeGroup>=0&&activeGroup!=group)continue;
        int inGroup=0;for(int i=0;i<count;++i)if(group_of(i)==group&&matches(review::component(i).name,search))++inGroup;
        if(!inGroup)continue;
        ImGui::SeparatorText(groups[group]);
        ImGui::PushID(group);
        if(ImGui::BeginTable("##catalog",cols,flags)){
            int n=0;
            for(int i=0;i<count;++i){
                if(group_of(i)!=group||!matches(review::component(i).name,search))continue;
                ImGui::TableNextColumn();
                if(tile(i)){*outIndex=i;ImGui::EndTable();ImGui::PopID();return true;}
                ++n;
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }
    return false;
}
}
