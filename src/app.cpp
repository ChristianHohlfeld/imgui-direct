#include "app.hpp"
#include "state_gallery.hpp"
#include "component_catalog.hpp"
#include "ui/components.hpp"
#include "ui/extended.hpp"
#include "ui/essentials.hpp"
#include "ui/theme.hpp"
#include "ui/scroll.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

#ifdef __EMSCRIPTEN__
extern "C" void browser_input_frame();
extern "C" void browser_input_region(float,float,float,float,int);
extern "C" void browser_qa_state(int page,int clicks);
extern "C" void browser_qa_item(int id,float x,float y,float w,float h);
extern "C" void browser_qa_field(int id,const char* value);
extern "C" void browser_qa_catalog(int,const char*,unsigned,int);
#endif

namespace {
struct Demo {
    int page=9, choice=0, plan=1, clicks=0, listSelected=-1;
    bool dark=false, notify=true, agreed=true, invalid=false, modal=false;
    float volume=.65f, progress=.72f;
    char name[128]="Alex Morgan";
    char email[128]="alex@example.com";
    char note[320]="Ein kurzer Gedanke ...";
} d;
float safeTop=0,safeRight=0,safeBottom=0,safeLeft=0,keyboardHeight=0;
bool focusInput=false;
ui::ToastQueue notifications;
void mark([[maybe_unused]] int id){
#ifdef __EMSCRIPTEN__
    const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
    browser_qa_item(id,a.x,a.y,b.x-a.x,b.y-a.y);
#endif
}
bool gallery_navigation(){
    bool changed=false;
    if(d.page==8||d.page==9){
        ui::Options nav;nav.variant=ui::Variant::Ghost;nav.size=ui::Size::SM;
        if(ui::Button(d.page==8?"Alle Komponenten":"Beispiele",nav)){
            d.page=d.page==8?9:0;changed=true;
        }
        mark(0);
        ImGui::SameLine();
        if(ui::Button(d.page==8?"Beispiele":"Zustände testen",nav)){
            d.page=d.page==8?0:8;changed=true;
        }
        mark(8);
        if(d.page==9){
            ImGui::SameLine();
            const bool dark=ui::mode()==ui::Mode::Dark;
            if(ui::Button(dark?"Hell":"Dunkel",ui::Variant::Outline))
                ui::set_mode(dark?ui::Mode::Light:ui::Mode::Dark);
            mark(910);
        }
        return changed;
    }
    static const char* names[]={"Buttons","Eingabe","Design","Liste","Menüs","Daten","Editor","Layout"};
    const float gap=ImGui::GetStyle().ItemSpacing.x;
    const float per=std::max(1.f,(ImGui::GetContentRegionAvail().x-gap*3)/4);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{5.f*ui::scale(),6.f*ui::scale()});
    for(int i=0;i<8;++i){
        if(i%4)ImGui::SameLine();
        ImGui::PushID(i);ui::Options opt;opt.size=ui::Size::SM;opt.variant=ui::Variant::Ghost;opt.state.selected=d.page==i;
        if(ui::Button(names[i],opt,{per,0})){changed=d.page!=i;d.page=i;}
        mark(i);ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ui::Options all;all.variant=ui::Variant::Outline;all.fullWidth=true;all.size=ui::Size::SM;all.state.selected=d.page==8;
    if(ui::Button("Alle Komponenten · Galerie",all)){changed=d.page!=9;d.page=9;}mark(9);
    ui::Options audit; audit.variant=ui::Variant::Ghost;audit.size=ui::Size::SM;
    if(ui::Button("Zustandsprüfung",audit)){changed=d.page!=8;d.page=8;}mark(8);
    return changed;
}

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
        mark(100);
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
        ui::Input("E-Mail",d.email,sizeof(d.email),"name@beispiel.de");
#ifdef __EMSCRIPTEN__
        browser_qa_field(0,d.name);browser_qa_field(1,d.email);
#endif
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
#ifdef __EMSCRIPTEN__
        {const auto p=ImGui::GetWindowPos(),s=ImGui::GetWindowSize();browser_qa_item(102,p.x,p.y,s.x,s.y);}
#endif
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
void navigation_demo(){
    introduction("Navigation","Menüs, Pfade und Abläufe. Zustand bleibt bei der Anwendung.");
    static int selected=0,crumb=-1,page=0,expanded=0;
    const char* path[]={"Projekt","Szenen","Innenhof"};
    ui::Breadcrumbs("breadcrumbs",path,&crumb);
    if(crumb>=0){ImGui::Text("Gewählter Pfad: %s",path[crumb]);}
    ui::Divider("Menü");
    const ui::NavItem items[]={{"Szene"},{"Assets"},{"Einstellungen"},{"Gesperrt",true}};
    ui::Navigation("menu",items,&selected,true);
    if(ui::BeginDropdown("Aktionen")){
        if(ui::MenuItem("Speichern","Ctrl+S"))notifications.push("Projekt gespeichert",ui::Status::Success,ImGui::GetTime());
        ui::MenuItem("Exportieren",nullptr,false,false);ui::EndDropdown();
    }
    ui::Tooltip("Die Export-Aktion ist in diesem Beispiel deaktiviert.");
    ui::Divider("Seiten");ui::Pagination("pagination",127,20,&page);
    ui::Divider("Aufklappbare Bereiche");
    if(ui::Accordion("Transform",0,&expanded))ui::Muted("Position, Rotation und Skalierung.");
    if(ui::Accordion("Darstellung",1,&expanded))ui::Muted("Material, Sichtbarkeit und Schatten.");
    ui::Divider("Schritte");const char* steps[]={"Auswählen","Prüfen","Übernehmen"};ui::Steps("steps",steps,selected%3);
    ui::Kbd("Ctrl + S");
}
void essentials_demo(bool narrow){
    if(ui::BeginCard("##native-essentials",ui::CardOptions{ui::Size::MD,true})){
        introduction("DaisyUI Must-haves","Native, responsiv, mit echter Tastatur- und Touch-Bedienung.");
        static int active=1,stars=3;
        static const char* modes[]={"Auswahl","Bearbeiten","Ansicht"};
        ui::Muted("Segmentierte Auswahl");
        ui::Segmented("Werkzeugmodus",modes,&active);
        space();
        ui::Rating("Bewertung",&stars,5);
        space(5);
        if(ui::Link("Details zum Projekt",ui::Tone::Primary))
            notifications.push("Link-Aktion an die Anwendung übergeben",ui::Status::Info,ImGui::GetTime());
        space(6);
        ui::Badge("Primär",{ui::Tone::Primary,ui::Surface::Solid,ui::Size::MD});
        ImGui::SameLine();
        ui::Badge("Outline",{ui::Tone::Secondary,ui::Surface::Outline,ui::Size::MD});
        if(narrow)space(3);else ImGui::SameLine();
        ui::Badge("Soft",{ui::Tone::Accent,ui::Surface::Soft,ui::Size::MD});
        space(7);
        ui::Alert("Hinweis","Semantische Farben, Rundungen und Größen folgen dem daisyUI-Base-Schema.",
                  {ui::Tone::Info,ui::Surface::Soft});
    }
    ui::EndCard();
}
void data_demo(){
    introduction("Daten","Tabellen, Status und Hierarchien für echte Werkzeuge.");
    ui::Stat("Szenenobjekte","1.024","Nur sichtbare Listenzeilen werden aufgebaut.");
    ui::StatusDot(ui::Status::Success,"Szene geladen");
    ui::Avatar("CH");ImGui::SameLine();ui::Badge("Bearbeitung",ui::Status::Info);
    const ui::TableColumn columns[]={{"Objekt"},{"Typ"},{"Status"}};ui::TableOptions opts;opts.height=210;
    if(ui::BeginTable("objects",columns,opts)){
        for(int i=0;i<6;++i){ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::Text("Objekt %d",i+1);
            ImGui::TableNextColumn();ImGui::TextUnformatted(i%2?"Licht":"Mesh");
            ImGui::TableNextColumn();ui::StatusDot(ui::Status::Success,"Bereit");}
        ui::EndTable();
    }
    if(ui::TreeNode("scene","Innenhof")){
        if(ui::TreeNode("camera","Kamera",false,true))ui::EndTreeNode();
        if(ui::TreeNode("objects","Objekte")){
            if(ui::TreeNode("building","Gebäude",true,true))ui::EndTreeNode();
            ui::EndTreeNode();
        }ui::EndTreeNode();
    }
    ui::TimelineItem("Projekt geöffnet","Die Szene ist bereit.",ui::Status::Success);
    ui::TimelineItem("Material geändert","Noch nicht gespeichert.",ui::Status::Warning);
    ui::Divider("Ladezustände");ui::Loading("loading");ImGui::SameLine();ui::RadialProgress("radial",.72f);
    ui::Skeleton("placeholder",{0,22});
    if(ui::Button("Benachrichtigung anzeigen"))notifications.push("Änderung übernommen",ui::Status::Success,ImGui::GetTime());
    mark(101);
    ui::Divider("Chat-Bubbles");
    ui::ChatBubble("Die Szene wurde geladen.",false,ui::Tone::Base,"System","Gerade eben");
    ui::ChatBubble("Danke! Bitte Vorschau öffnen.",true,ui::Tone::Primary,"Du","Gesendet");
}
void editor_demo(){
    introduction("Editor-Eingaben","C++-Daten direkt bearbeiten, ohne UI-seitige Datenkopie.");
    static float position[3]={0,1,0},rotation[3]={0,0,0},color[4]={.2f,.5f,.9f,1},mass=10,low=2,high=8;
    static int layers=1;static bool locked=false;static char asset[96]="Innenhof";
    ui::Switch("Schreibgeschützt",&locked);
    ui::BeginFieldset("transform","Transform",locked);
    ui::Input("Name",asset,sizeof(asset));
    ui::Vector3("Position",position);ui::Vector3("Rotation",rotation);
    ui::Number("Masse",&mass,.1f,0,10000);ui::Integer("Ebenen",&layers);
    ui::ColorEdit("Materialfarbe",color);ui::Range("Entfernung",&low,&high,0,100);
    ui::EndFieldset();
    ui::Field invalid;invalid.invalid=true;invalid.help="Der Name darf nicht leer sein.";
    static char empty[32]="";ui::Input("Validierungsbeispiel",empty,sizeof(empty),"Pflichtfeld",invalid);
    ui::Field help;help.help="Die Anwendung stellt die Dateiauswahl bereit.";
    ui::FileInput("Datei","Kein Dateidialog in der Demo",{},help);
}
void layout_demo(){
    introduction("Layout","Verstellbare Bereiche und zusammengesetzte Oberflächen.");
    static float ratio=.35f;static bool drawer=true;
    ui::Switch("Seitenbereich anzeigen",&drawer);
    if(drawer){
        const bool visible=ui::BeginSplit("workspace",&ratio,{0,300},ImGui::GetContentRegionAvail().x<520);
        if(visible){ui::Heading("Szene");if(ui::TreeNode("root","Innenhof")){
            if(ui::TreeNode("mesh","Gebäude",true,true))ui::EndTreeNode();
            ui::EndTreeNode();}}
        if(ui::NextSplit()){ui::Heading("Eigenschaften");ui::Muted("Der Trenner ist mit Maus, Touch und Pfeiltasten bedienbar.");
            ui::Progress("Laden",.85f);}
        ui::EndSplit();
    }else ui::Alert("Mehr Platz","Der Seitenbereich ist ausgeblendet.");
    space();ui::Divider("Zusammengefasste Aktionen");
    if(ui::Button("Speichern"))notifications.push("Gespeichert",ui::Status::Success,ImGui::GetTime());
    ImGui::SameLine();if(ui::Button("Zurücksetzen",ui::Variant::Outline))ratio=.35f;
}
void theme(bool narrow){
    introduction("Erscheinungsbild","Eine gemeinsame Palette für jedes Element.");
    if(ui::BeginCard("##theme")){
        ui::Heading("daisyUI Base");
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
void app::review_configure(int component,int state,int dark,int size){
    d.page=8;review::select(component,static_cast<review::State>(state));
    ui::set_mode(dark?ui::Mode::Dark:ui::Mode::Light);
    review::set_size(static_cast<ui::Size>(std::clamp(size,0,4)));
}
void app::setup(){
    ui::initialize();ui::set_mode(ui::Mode::Light);
#ifdef __EMSCRIPTEN__
    for(int i=0;i<review::count();++i){const auto& c=review::component(i);browser_qa_catalog(i,c.name,c.states,c.interactive);}
    ui::SetInputObserver([](ImVec2 a,ImVec2 b,bool active,void*){
        browser_input_region(a.x,a.y,b.x-a.x,b.y-a.y,active?1:0);
    });
#endif
}
void app::set_touch(bool value){
    if(value)ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_IsTouchScreen;
    else ImGui::GetIO().ConfigFlags&=~ImGuiConfigFlags_IsTouchScreen;
    ui::set_touch(value);
}
void app::safe_area(float a,float b,float c,float e){safeTop=a;safeRight=b;safeBottom=c;safeLeft=e;}
void app::keyboard_height(float h){keyboardHeight=std::max(0.f,h);}
void app::request_text_focus(){
    // The canvas touch selects the C++ field. Native IME focus must not
    // steal that selection or switch the gallery page.
}
void app::zoom(float ratio){ui::zoom(ratio);}
void app::draw(){
    ui::begin_frame();
    ui::scroll::begin_frame();
    ui::scroll::dispatch_before_begin(); // same-frame native pixel response
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
    ui::Title(d.page==9?"imgui-direct":"Komponenten");
    if(d.page!=9)ui::Muted("Einfach. Einheitlich. Für alle Bildschirme.");
    else ui::Muted("Einfach. Einheitlich. Für alle Bildschirme.");
    space(d.page==9?3.f:8.f);
    const bool pageChanged=gallery_navigation();
    space(d.page==9?3.f:9.f);
#ifdef __EMSCRIPTEN__
    browser_input_frame();
#endif
    if(pageChanged){
        ImGui::SetNextWindowScroll({0,0});
        if(d.page==9)gallery::reset_view_scroll();
    }
    ImGui::BeginChild("##gallery-content",{0,0},ImGuiChildFlags_NavFlattened);
    // The background stays fixed; only content vertices get a brief elastic
    // translation when a pixel/touch gesture exceeds a real scroll boundary.
    const int contentVertexStart=ImGui::GetWindowDrawList()->VtxBuffer.Size;
    if(d.page==0){
        introduction("Grundelemente","Alles, was eine klare Oberfläche braucht.");
        const bool columns=!narrow&&ImGui::GetContentRegionAvail().x>720.f;
        if(columns)ImGui::Columns(2,"##two",false);
        buttons(narrow);
        if(columns)ImGui::NextColumn();else space(10);
        sample(narrow);
        if(columns)ImGui::Columns(1);
        space(12);
        feedback(narrow);space(12);essentials_demo(narrow);
    }else if(d.page==1){
        introduction("Eingaben","Einheitliche Felder mit echten Interaktionszuständen.");
        const bool columns=!narrow&&ImGui::GetContentRegionAvail().x>720.f;
        if(columns)ImGui::Columns(2,"##forms",false);
        fields(narrow);
        if(columns)ImGui::NextColumn();else space(10);
        choices();
        if(columns)ImGui::Columns(1);
    }else if(d.page==2)theme(narrow);
    else if(d.page==3)virtual_list();
    else if(d.page==4)navigation_demo();
    else if(d.page==5)data_demo();
    else if(d.page==6)editor_demo();
    else if(d.page==7)layout_demo();
    else if(d.page==8)review::draw();
    else {
        int open=-1;
        if(gallery::draw_index(&open)){review::select(open,review::State::Normal);d.page=8;ImGui::SetNextWindowScroll({0,0});}
    }
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
    ui::scroll::bounce_after_scroll();
    ui::scroll::render_bounce(contentVertexStart);
    ImGui::EndChild();
    ui::scroll::end_frame();
#ifdef __EMSCRIPTEN__
    // Test instrumentation only, not a second UI or user-visible controls.
    browser_qa_state(d.page,d.clicks);
#endif
    ImGui::End();
    ui::Toasts("##gallery-toasts",notifications,ImGui::GetTime());
}
