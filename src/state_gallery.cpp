#include "state_gallery.hpp"
#include "ui/scroll.hpp"
#include "imgui_internal.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>

#ifdef __EMSCRIPTEN__
extern "C" void browser_qa_item(int,float,float,float,float);
extern "C" void browser_qa_review(int,int,int,int,int,int,int,float,float,float,float);
#endif
namespace review {
namespace {
using S=State;
constexpr auto normal=bit(S::Normal),disabled=bit(S::Disabled),selected=bit(S::Selected),empty=bit(S::Empty),
    open=bit(S::Open),closed=bit(S::Closed),minimum=bit(S::Minimum),maximum=bit(S::Maximum),longText=bit(S::LongText);
constexpr auto feedback=bit(S::Error)|bit(S::Warning)|bit(S::Success);
constexpr auto field=normal|disabled|bit(S::ReadOnly)|bit(S::Loading)|bit(S::Required)|feedback;
constexpr Component components[]={
 {"Button",(field&~bit(S::Required))|selected|longText,true},{"Input",field|empty|longText,true},
 {"Textarea",field|empty|longText,true},{"Select",field|selected|empty,true},
 {"Checkbox",field|selected|bit(S::Mixed),true},{"Radio",field|selected,true},
 {"Switch",field|selected|longText,true},{"Slider",field|minimum|maximum,true},
 {"Number",field|minimum|maximum,true},{"Integer",field|minimum|maximum,true},
 {"Vector3",field,true},{"ColorEdit",field,true},{"Range",field|minimum|maximum,true},
 {"FileInput",field|empty|selected,true},{"Tabs",normal|selected|disabled|longText,true},
 {"Navigation",normal|selected|disabled|longText,true},{"Breadcrumbs",normal|empty|longText,true},
 {"Pagination",normal|empty|minimum|maximum|disabled,true},
 {"Accordion",normal|open|closed|disabled,true},{"Collapse",normal|open|closed|disabled,true},
 {"Dropdown",normal|open|closed|disabled,true},{"MenuItem",normal|selected|disabled,true},
 {"ContextMenu",normal|open|closed|disabled,true},{"Modal",normal|open|closed|longText,true},
 {"Card",normal|empty|longText,false},{"Fieldset",normal|disabled|bit(S::ReadOnly)|feedback,true},
 {"Table",normal|empty|selected|disabled,true},{"TreeNode",normal|open|closed|selected|disabled,true},
 {"VirtualList",normal|empty|selected|disabled,true},{"Split",normal|minimum|maximum|disabled,true},
 {"Badge",normal|feedback|longText,false},{"Alert",normal|feedback|longText,false},
 {"Progress",normal|minimum|maximum|feedback,false},{"RadialProgress",normal|minimum|maximum|feedback,false},
 {"Loading",normal,false},{"Skeleton",normal,false},{"Stat",normal|empty|longText,false},
 {"StatusDot",normal|feedback,false},{"Steps",normal|empty|minimum|maximum,false},
 {"TimelineItem",normal|feedback|longText,false},{"Avatar",normal|empty|longText,false},
 {"Kbd",normal|longText,false},{"Tooltip",normal|disabled|longText,true},
 {"Label",normal|bit(S::Required)|longText,false},{"Validation",normal|feedback,false},
 {"Divider",normal|empty|longText,false},{"Typography",normal|longText,false},{"Toast",normal|feedback|empty|longText,true}
};
constexpr const char* stateNames[]={"Normal / interaktiv","Ausgewählt / an","Deaktiviert","Schreibgeschützt","Laden",
    "Fehler","Warnung","Erfolg","Leer","Unbestimmt","Geöffnet","Geschlossen","Minimum","Maximum","Langer Inhalt","Pflichtfeld"};
int current=0,changesCount=0,generation=0;S state=S::Normal;
ui::ControlInfo primaryInfo{};
struct Model {
 char text[1024]="Innenhof",file[64]="";
 bool checked=false,expanded=false,modal=false;int choice=0,page=2,accordion=-1,integer=3,row=-1;
 float value=.45f,vector[3]={0,1,0},rgba[4]={.2f,.5f,.8f,1},low=.2f,high=.8f,ratio=.4f;
} model;
ui::ToastQueue toastQueue;
ui::Size size=ui::Size::MD;
ui::Variant variant=ui::Variant::Primary;
bool closePending=false;
const char* longSample="Ein langer, absichtlich umbrechender Inhalt für schmale Ansichten und große Schrift. Keine Information darf außerhalb des Panels verschwinden.";
bool is(S s){return state==s;}
ui::Status status(){return is(S::Error)?ui::Status::Error:is(S::Warning)?ui::Status::Warning:is(S::Success)?ui::Status::Success:ui::Status::Info;}
ui::Field options(){
 ui::Field f;f.size=size;f.disabled=is(S::Disabled);f.readOnly=is(S::ReadOnly);f.loading=is(S::Loading);
 f.invalid=is(S::Error);f.indeterminate=is(S::Mixed);f.required=is(S::Required);
 f.validation=is(S::Warning)?ui::ValidationState::Warning:is(S::Success)?ui::ValidationState::Success:ui::ValidationState::None;
 f.help=f.invalid?"Bitte den Wert prüfen.":is(S::Warning)?"Dieser Wert benötigt Aufmerksamkeit.":is(S::Success)?"Der Wert ist gültig.":f.readOnly?((current==1||current==2)?"Auswahl und Kopieren erlaubt; keine Änderung.":"Keine Änderung erlaubt."):nullptr;
 return f;
}
void note(bool changed=false){primaryInfo=ui::last_control();if(changed)++changesCount;}
void direct_note(bool changed=false){
 const bool touch=ImGui::GetIO().MouseSource==ImGuiMouseSource_TouchScreen;
 primaryInfo={{!touch&&ImGui::IsItemHovered(),ImGui::IsItemActive(),ImGui::IsItemFocused(),ImGui::IsItemActivated(),is(S::Disabled),ImGui::IsItemDeactivated()},ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),ImGui::GetItemID()};
 if(changed)++changesCount;
}
void list_row(int row,void*){
 char label[64];std::snprintf(label,sizeof(label),"Objekt %05d",row);
 bool clicked=ImGui::Selectable(label,model.row==row,0,{0,ui::control_height(size)-ImGui::GetStyle().ItemSpacing.y});
 if(row==0||ImGui::IsItemFocused()||ImGui::IsItemHovered())direct_note();
 if(clicked){model.row=row;++changesCount;}
}
void reset(){
 model=Model{};changesCount=0;++generation;closePending=true;
 if(is(S::Empty))model.text[0]=0;
 if(is(S::LongText))std::snprintf(model.text,sizeof(model.text),"%s",longSample);
 model.checked=is(S::Selected);model.choice=is(S::Empty)?-1:is(S::Selected)?1:0;
 model.row=is(S::Selected)?0:-1;model.expanded=is(S::Open);model.accordion=is(S::Open)?0:-1;
 if(is(S::Minimum)){model.value=0;model.integer=0;model.ratio=.1f;model.page=0;model.low=model.high=0;}
 if(is(S::Maximum)){model.value=1;model.integer=100;model.ratio=.9f;model.page=12;model.low=model.high=1;}
 toastQueue=ui::ToastQueue{};
}
void mark(int id){
#ifdef __EMSCRIPTEN__
 auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();browser_qa_item(id,a.x,a.y,b.x-a.x,b.y-a.y);
#else
 (void)id;
#endif
}
}
void set_size(ui::Size next){size=next;}
void set_variant(ui::Variant next){variant=next;}
int count(){return static_cast<int>(std::size(components));}
const Component& component(int index){return components[std::clamp(index,0,count()-1)];}
const char* state_name(S s){return stateNames[std::clamp(static_cast<int>(s),0,static_cast<int>(S::Count)-1)];}
bool supports(int index,S s){return index>=0&&index<count()&&static_cast<int>(s)>=0&&s<S::Count&&(components[index].states&bit(s));}
void select(int index,S s){
 current=std::clamp(index,0,count()-1);state=supports(current,s)?s:S::Normal;reset();
}
int selected_component(){return current;}
S selected_state(){return state;}
int changes(){return changesCount;}
ui::ControlInfo primary(){return primaryInfo;}
void preview(){
 const bool initial=closePending;
 if(closePending){
   if(!GImGui->OpenPopupStack.empty())ImGui::ClosePopupToLevel(0,true);
   ImGui::ClearActiveID();
   // Replacing the entire preview invalidates its old NavId. Reinitialize
   // native navigation instead of keeping Tab anchored to a vanished item.
   ImGui::FocusWindow(ImGui::GetCurrentWindow());
   ImGui::NavInitWindow(ImGui::GetCurrentWindow(),true);
   closePending=false;
 }
 primaryInfo={};ImGui::PushID(current);ImGui::PushID(static_cast<int>(state));
 auto f=options();const char* choices[]={"Szene","Assets","Einstellungen"};
 const char* labels[]={"Szene","Assets","Ansicht"};
 ImGui::BeginDisabled(is(S::Disabled));
 switch(current){
 case 0:{ui::Options o;o.size=size;o.fullWidth=true;o.state={f.disabled,f.loading,is(S::Selected),f.invalid,f.readOnly};
   o.variant=is(S::Warning)?ui::Variant::Warning:is(S::Success)?ui::Variant::Success:variant;
   note(ui::Button(is(S::LongText)?longSample:"Aktion ausführen",o));break;}
 case 1:note(ui::Input("Bezeichnung",model.text,sizeof(model.text),"Name eingeben",f));break;
 case 2:note(ui::Textarea("Beschreibung",model.text,sizeof(model.text),f,3));break;
 case 3:note(ui::Select("Bereich",choices,is(S::Empty)?0:3,&model.choice,f));break;
 case 4:{bool changed=ui::Checkbox("Sichtbar",&model.checked,f);note(changed);break;}
 case 5:note(ui::Radio("Auswahl",1,&model.choice,f));break;
 case 6:note(ui::Switch(is(S::LongText)?longSample:"Benachrichtigungen",&model.checked,f));break;
 case 7:note(ui::Slider("Deckkraft",&model.value,0,1,"%.2f",f));break;
 case 8:note(ui::Number("Masse",&model.value,.01f,0,1,f));break;
 case 9:note(ui::Integer("Anzahl",&model.integer,1,f));break;
 case 10:note(ui::Vector3("Position",model.vector,.1f,f));break;
 case 11:note(ui::ColorEdit("Materialfarbe",model.rgba,f));break;
 case 12:note(ui::Range("Bereich",&model.low,&model.high,0,1,f));break;
 case 13:{
   if(is(S::Selected)&&!*model.file)std::snprintf(model.file,sizeof(model.file),"auswahl.mesh");
   ui::FilePicker picker{[](void*){std::snprintf(model.file,sizeof(model.file),"beispiel.mesh");},nullptr};
   note(ui::FileInput("Asset",model.file,picker,f));ui::Muted("Demo-Callback; der Host stellt einen echten Dateidialog bereit.");break;}
 case 14:{const char* longLabels[]={"Langer Szenenname","Materialbibliothek","Projekteinstellungen"};
   note(ui::Tabs("tabs",is(S::LongText)?longLabels:labels,3,&model.choice,size));break;}
 case 15:{const ui::NavItem items[]={{"Szene"},{"Nicht verfügbar",true},{is(S::LongText)?longSample:"Assets"}};
   note(ui::Navigation("navigation",items,&model.choice,true,size));break;}
 case 16:{int clicked=-1;const char* longPath[]={"Projekt","Lange Szenenbezeichnung","Eigenschaften"};
   note(ui::Breadcrumbs("path",is(S::Empty)?std::span<const char* const>{}:std::span<const char* const>(is(S::LongText)?longPath:labels,3),&clicked));
   if(is(S::Empty))ui::Muted("Kein Pfad vorhanden.");
   break;}
 case 17:note(ui::Pagination("pages",is(S::Empty)?0:127,10,&model.page));break;
 case 18:{const bool before=model.accordion==0;bool visible=ui::Accordion("Eigenschaften",0,&model.accordion,f.disabled);direct_note(visible!=before);
   if(visible)ui::Input("Name",model.text,sizeof(model.text));
   break;}
 case 19:{const bool before=model.expanded;bool visible=ui::Collapse("Details",&model.expanded,f.disabled);direct_note(visible!=before);
   if(visible)ui::Muted("Position, Rotation und Skalierung.");
   break;}
 case 20:{if(is(S::Open)&&model.choice==0){ImGui::OpenPopup("Aktionen");model.choice=1;}
   bool opened=ui::BeginDropdown("Aktionen");note();
   if(opened){if(ui::MenuItem("Speichern","Ctrl+S"))++changesCount;ui::MenuItem("Ausgewählt",nullptr,true);ui::MenuItem("Gesperrt",nullptr,false,false);ui::EndDropdown();}break;}
 case 21:{bool clicked=ui::MenuItem("Objekt auswählen","Ctrl+1",model.checked,!f.disabled);direct_note(clicked);if(clicked)model.checked=!model.checked;break;}
 case 22:{if(ui::Button("Kontextmenü öffnen")){ImGui::OpenPopup("context");++changesCount;}note();
   if(is(S::Open)&&model.choice==0){ImGui::OpenPopup("context");model.choice=1;}
   if(ui::BeginContextMenu("context")){if(ui::MenuItem("Duplizieren"))++changesCount;ui::MenuItem("Entfernen",nullptr,false,false);ui::EndContextMenu();}break;}
 case 23:{if(ui::Button("Dialog öffnen")){model.modal=true;ui::OpenModal("Bestätigung");++changesCount;}note();
   if(is(S::Open)&&model.choice==0){model.modal=true;ui::OpenModal("Bestätigung");model.choice=1;}
   if(ui::BeginModal("Bestätigung",&model.modal)){ui::Heading("Änderung übernehmen?");ui::Muted(is(S::LongText)?longSample:"Eine echte modale Bestätigung mit Escape und Fokusbindung.");
     if(ui::Button("Übernehmen")){++changesCount;ImGui::CloseCurrentPopup();}ImGui::SameLine();
     if(ui::Button("Abbrechen",ui::Variant::Outline))ImGui::CloseCurrentPopup();
     ui::EndModal();}break;}
 case 24:if(ui::BeginCard("card")){if(is(S::Empty))ui::Muted("Keine Inhalte.");else{ui::Heading("Projekt");ui::Muted(is(S::LongText)?longSample:"Inhalte gruppieren, ohne eigene Zustandsmaschine.");}}ui::EndCard();break;
 case 25:ui::BeginFieldset("fieldset","Eigenschaften",f.disabled);note(ui::Input("Name",model.text,sizeof(model.text),"",f));ui::Checkbox("Sichtbar",&model.checked,f);ui::EndFieldset();break;
 case 26:{const ui::TableColumn columns[]={{"Objekt"},{"Wert"}};ui::TableOptions o;o.height=150;o.sortable=true;
   if(ui::BeginTable("table",columns,o)){
     auto* sort=ImGui::TableGetSortSpecs();bool descending=sort&&sort->SpecsCount&&sort->Specs[0].SortDirection==ImGuiSortDirection_Descending;
     if(sort)sort->SpecsDirty=false;
     if(is(S::Empty)){ImGui::TableNextRow();ImGui::TableNextColumn();ui::Muted("Keine Datensätze.");}
     else for(int i=0;i<5;++i){int row=descending?4-i:i;ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::PushID(row);
       char text[32];std::snprintf(text,sizeof(text),"Objekt %d",row+1);bool clicked=ImGui::Selectable(text,model.row==row,ImGuiSelectableFlags_SpanAllColumns);
       if(i==0||ImGui::IsItemFocused()||ImGui::IsItemHovered())direct_note();
       if(clicked){model.row=row;++changesCount;}ImGui::TableNextColumn();ImGui::Text("%d",row+1);ImGui::PopID();}
     ui::EndTable();}break;}
 case 27:{if(initial)ImGui::SetNextItemOpen(is(S::Open),ImGuiCond_Always);
   bool expanded=ui::TreeNode("root","Szene",is(S::Selected));direct_note();
   if(expanded){if(ui::TreeNode("leaf","Kamera",false,true))ui::EndTreeNode();ui::EndTreeNode();}break;}
 case 28:if(is(S::Empty))ui::Muted("Keine Objekte.");else ui::VirtualList("list",100000,160,ui::control_height(size),list_row);break;
 case 29:{if(ui::BeginSplit("split",&model.ratio,{0,180},ImGui::GetContentRegionAvail().x<500))ui::Muted("Szene");
   const bool second=ui::NextSplit();note();if(second)ui::Muted("Eigenschaften");
   ui::EndSplit();break;}
 case 30:ui::Badge(is(S::LongText)?"Langer Status eines Projekts":"Status",status());break;
 case 31:ui::Alert("Hinweis",is(S::LongText)?longSample:"Änderung gespeichert.",status());break;
 case 32:ui::Progress("Fortschritt",model.value,status());break;
 case 33:ui::RadialProgress("radial",model.value,size,status());break;
 case 34:ui::Loading("busy",size);ui::Muted("Laufender Vorgang; keine Aktion auslösbar.");break;
 case 35:ui::Skeleton("one",{0,control_height(size)});ui::Skeleton("two",{0,16});break;
 case 36:ui::Stat(is(S::LongText)?longSample:"Objekte",is(S::Empty)?"—":"1.024","In der aktiven Szene");break;
 case 37:ui::StatusDot(status(),"Status");break;
 case 38:ui::Steps("steps",labels,is(S::Empty)?-1:is(S::Maximum)?2:is(S::Minimum)?0:1);break;
 case 39:ui::TimelineItem("Projekt geöffnet",is(S::LongText)?longSample:"Zeitpunkt und Beschreibung des Ereignisses",status());break;
 case 40:ui::Avatar(is(S::Empty)?"":is(S::LongText)?"LONG":"CH",size);break;
 case 41:ui::Kbd(is(S::LongText)?"Ctrl + Shift + Alt + S":"Ctrl + S");break;
 case 42:ui::Button("Aktion mit Hinweis");note();ui::Tooltip(is(S::LongText)?longSample:"Ein zusätzlicher Hinweis per Hover oder Tastaturfokus.");break;
 case 43:ui::Label(is(S::LongText)?longSample:"Feldbezeichnung",is(S::Required));break;
 case 44:ui::Validation(is(S::Error)?"Der Wert ist ungültig.":is(S::Warning)?"Wert prüfen.":"Der Wert ist gültig.",is(S::Error)?ui::ValidationState::Error:is(S::Warning)?ui::ValidationState::Warning:is(S::Success)?ui::ValidationState::Success:ui::ValidationState::None);break;
 case 45:if(is(S::Empty))ui::Divider();else ui::Divider(is(S::LongText)?"Ein längerer Abschnittstitel":"Abschnitt");break;
 case 46:ui::Title("Titel");ui::Heading("Überschrift");ui::Muted(is(S::LongText)?longSample:"Normaler Fließtext und ergänzende Information.");break;
 case 47:{if(!is(S::Empty)&&model.choice==0){toastQueue.push(is(S::LongText)?longSample:"Gespeichert",status(),ImGui::GetTime(),0);model.choice=1;}
   if(ui::Button("Benachrichtigung auslösen")){toastQueue.push(is(S::LongText)?longSample:"Gespeichert",status(),ImGui::GetTime(),4);++changesCount;}note();
   ui::Muted(is(S::Empty)?"Die Warteschlange ist leer.":"Echte Warteschlange: schließbar, nach vier Sekunden abgelaufen.");ui::Toasts("##review-toast",toastQueue,ImGui::GetTime());break;}
 }
 ImGui::EndDisabled();ImGui::PopID();ImGui::PopID();
}
void draw(){
 ui::Heading("Zustandsprüfung");
 ui::Muted("Jede Komponente einzeln. Hover, Druck und Fokus entstehen durch echte Bedienung.");
 int next=current;
 ImGui::SetNextItemWidth(-1);
 if(ImGui::BeginCombo("##component",component(current).name)){
   for(int i=0;i<count();++i)if(ImGui::Selectable(component(i).name,current==i))next=i;
   ImGui::EndCombo();
 }
 mark(200);
 if(next!=current)select(next,S::Normal);
 ImGui::SetNextItemWidth(-1);
 if(ImGui::BeginCombo("##state",state_name(state))){
   for(int i=0;i<static_cast<int>(S::Count);++i)if(supports(current,static_cast<S>(i))&&ImGui::Selectable(state_name(static_cast<S>(i)),state==static_cast<S>(i)))select(current,static_cast<S>(i));
   ImGui::EndCombo();
 }
 mark(201);
 bool dark=ui::mode()==ui::Mode::Dark;
 if(ui::Checkbox("Dunkel",&dark))ui::set_mode(dark?ui::Mode::Dark:ui::Mode::Light);
 mark(202);
 ImGui::SameLine();if(ui::Button("Reset",ui::Variant::Outline))reset();mark(203);
 const char* sizes[]={"XS","SM","MD","LG","XL"};int sizeIndex=static_cast<int>(size);
 ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##size",&sizeIndex,sizes,5))size=static_cast<ui::Size>(sizeIndex);mark(204);
 if(current==0){
   const char* variants[]={"Primary","Secondary","Neutral","Outline","Ghost","Error","Success","Warning","Info","Soft"};
   int selected=static_cast<int>(variant);ImGui::SetNextItemWidth(-1);
   if(ImGui::Combo("##variant",&selected,variants,10))variant=static_cast<ui::Variant>(selected);
 }
 ui::Divider(component(current).name);
 ImGui::Text("%d / %d · %s",current+1,count(),state_name(state));
 preview();
 const auto info=primaryInfo;
 if(component(current).interactive){
   ImGui::Text("Änderungen: %d",changesCount);
   ImGui::TextWrapped("Hover %s · Aktiv %s · Fokus %s · Gesperrt %s",info.interaction.hovered?"ja":"nein",info.interaction.held?"ja":"nein",info.interaction.focused?"ja":"nein",info.interaction.disabled?"ja":"nein");
 }else ui::Muted("Anzeigeelement: kein künstlicher Hover-, Klick- oder Disabled-Zustand.");
#ifdef __EMSCRIPTEN__
 browser_qa_review(current,static_cast<int>(state),changesCount,info.interaction.hovered,info.interaction.held,info.interaction.focused,info.interaction.disabled,info.minimum.x,info.minimum.y,info.maximum.x-info.minimum.x,info.maximum.y-info.minimum.y);
#endif
}
}
