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
#endif

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
bool tile(int index){
    const auto& meta=review::component(index);
    const auto& pal=ui::palette();
    const float s=ui::scale(),radius=pal.metrics.radiusBox*s;
    const ImVec4 surface{pal.base100.r,pal.base100.g,pal.base100.b,1};
    const ImVec4 previewSurface{pal.base200.r,pal.base200.g,pal.base200.b,1};
    const ImVec4 border{pal.base300.r,pal.base300.g,pal.base300.b,1};
    ImGui::PushID(index);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,radius);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{14.f*s,12.f*s});
    ImGui::PushStyleColor(ImGuiCol_ChildBg,surface);
    ImGui::PushStyleColor(ImGuiCol_Border,border);
    bool opened=false;
    // A real preview is separate from the card's navigation action.
    // Native controls inside this preview remain clickable and keyboard-ready.
    if(ImGui::BeginChild("##tile",{0,226.f*s},ImGuiChildFlags_Borders|ImGuiChildFlags_NavFlattened,
                         ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        ImGui::TextUnformatted(meta.name);
        ImGui::SameLine();
        ImGui::TextDisabled("%02d",index+1);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,pal.metrics.radiusField*s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{12.f*s,10.f*s});
        ImGui::PushStyleColor(ImGuiCol_ChildBg,previewSurface);
        if(ImGui::BeginChild("##preview",{0,125.f*s},0,
                            ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
            sample(index);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();ImGui::PopStyleVar(2);
        ImGui::Dummy({0,3.f*s});
        ui::Options action;action.variant=ui::Variant::Ghost;action.size=ui::Size::SM;action.fullWidth=true;
        opened=ui::Button("Ansehen und Zustände",action);
#ifdef __EMSCRIPTEN__
        const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
        browser_qa_item(900+index,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);ImGui::PopStyleVar(2);ImGui::PopID();
    return opened;
}
int filter=0;
void filter_buttons(bool vertical,float width){
    const char* options[]={"Alle","Aktionen","Eingaben","Navigation","Layout","Feedback"};
    const float s=ui::scale();
    const float btnWidth=vertical?width:std::max(1.f,(width-2.f*6.f*s)/3.f);
    ImGui::PushID("catalog-filters");
    for(int i=0;i<6;++i){
        if(!vertical&&i%3)ImGui::SameLine();
        ImGui::PushID(i);
        ui::Options b;b.size=ui::Size::SM;b.variant=filter==i?ui::Variant::Soft:ui::Variant::Ghost;
        b.state.selected=filter==i;
        if(ui::Button(options[i],b,{btnWidth,0}))filter=i;
        ImGui::PopID();
    }
    ImGui::PopID();
}
bool draw_groups(int* outIndex){
    const int count=review::count();
    const int active=filter-1;
    int matchesCount=0;
    for(int i=0;i<count;++i)
        if((active<0||active==group_of(i))&&matches(review::component(i).name,search))
            ++matchesCount;
    ImGui::TextDisabled("%d / %d Komponenten",matchesCount,count);
    if(!matchesCount){
        ImGui::Dummy({0,20.f*ui::scale()});
        ui::Muted("Keine Treffer. Anderen Suchbegriff oder Kategorie wählen.");
        return false;
    }
    const float width=ImGui::GetContentRegionAvail().x;
    const int cols=width<540.f*ui::scale()?1:width<850.f*ui::scale()?2:3;
    const ImGuiTableFlags flags=ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,{5.f*ui::scale(),6.f*ui::scale()});
    for(int group=0;group<5;++group){
        if(active>=0&&active!=group)continue;
        int groupCount=0;
        for(int i=0;i<count;++i)
            if(group_of(i)==group&&matches(review::component(i).name,search))++groupCount;
        if(!groupCount)continue;
        ImGui::SeparatorText(groups[group]);
        ImGui::PushID(group);
        if(ImGui::BeginTable("##catalog",cols,flags)){
            for(int i=0;i<count;++i){
                if(group_of(i)!=group||!matches(review::component(i).name,search))continue;
                ImGui::TableNextColumn();
                if(tile(i)){
                    *outIndex=i;
                    ImGui::EndTable();ImGui::PopID();ImGui::PopStyleVar();
                    return true;
                }
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    return false;
}
} // namespace
bool draw_index(int* outIndex){
    if(!outIndex)return false;
    const float s=ui::scale(),width=ImGui::GetContentRegionAvail().x;
    // Docs-like browse-first layout: compact search above responsive
    // category navigation, with live previews in real native cards.
    ImGui::PushID("component-index");
    ui::Input("Komponente suchen",search,sizeof(search),"z. B. Button, Input, Tabs ...");
    ImGui::Dummy({0,4.f*s});
    bool opened=false;
    if(width>=850.f*s){
        if(ImGui::BeginTable("##browser",2,ImGuiTableFlags_SizingFixedFit|ImGuiTableFlags_NoSavedSettings)){
            ImGui::TableSetupColumn("Kategorien",ImGuiTableColumnFlags_WidthFixed,162.f*s);
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
        ImGui::Dummy({0,5.f*s});
        opened=draw_groups(outIndex);
    }
    ImGui::PopID();
    return opened;
}
} // namespace gallery
