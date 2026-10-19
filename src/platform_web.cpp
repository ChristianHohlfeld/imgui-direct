#include "app.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <cstring>
namespace {
double previous=0;
ImGuiKey mapkey(const char* k) {
    if(std::strncmp(k,"Key",3)==0 && std::strlen(k)==4 && k[3]>='A'&&k[3]<='Z') return (ImGuiKey)(ImGuiKey_A+k[3]-'A');
    if(std::strncmp(k,"Digit",5)==0 && std::strlen(k)==6 && k[5]>='0'&&k[5]<='9') return (ImGuiKey)(ImGuiKey_0+k[5]-'0');
    struct Key{const char* name; ImGuiKey key;};
    static const Key m[]={{"Tab",ImGuiKey_Tab},{"Enter",ImGuiKey_Enter},
        {"Escape",ImGuiKey_Escape},{"Backspace",ImGuiKey_Backspace},
        {"Delete",ImGuiKey_Delete},{"Space",ImGuiKey_Space},
        {"ArrowLeft",ImGuiKey_LeftArrow},{"ArrowRight",ImGuiKey_RightArrow},
        {"ArrowUp",ImGuiKey_UpArrow},{"ArrowDown",ImGuiKey_DownArrow},
        {"Home",ImGuiKey_Home},{"End",ImGuiKey_End},
        {"ShiftLeft",ImGuiKey_LeftShift},{"ShiftRight",ImGuiKey_RightShift},
        {"ControlLeft",ImGuiKey_LeftCtrl},{"ControlRight",ImGuiKey_RightCtrl},
        {"AltLeft",ImGuiKey_LeftAlt},{"AltRight",ImGuiKey_RightAlt}};
    for(const auto& it:m) if(std::strcmp(k,it.name)==0) return it.key;
    return ImGuiKey_None;
}
EM_BOOL keycb(int kind,const EmscriptenKeyboardEvent* e,void*) {
    auto& io=ImGui::GetIO();
    bool down=kind==EMSCRIPTEN_EVENT_KEYDOWN;
    io.AddKeyEvent(ImGuiMod_Ctrl,e->ctrlKey);
    io.AddKeyEvent(ImGuiMod_Shift,e->shiftKey);
    io.AddKeyEvent(ImGuiMod_Alt,e->altKey);
    io.AddKeyEvent(ImGuiMod_Super,e->metaKey);
    ImGuiKey k=mapkey(e->code); if(k!=ImGuiKey_None) io.AddKeyEvent(k,down);
    unsigned char first=(unsigned char)e->key[0];
    size_t len=std::strlen(e->key);
    if(down&&!e->ctrlKey&&!e->metaKey&&
       ((len==1 && first>=32 && first<127)||(first>=0xC2&&first<=0xF4&&len>=2&&len<=4)))
        io.AddInputCharactersUTF8(e->key);
    return io.WantCaptureKeyboard&&!e->ctrlKey&&!e->metaKey;
}
EM_BOOL mousecb(int kind,const EmscriptenMouseEvent* e,void*) {
    auto& io=ImGui::GetIO();
    io.AddMousePosEvent((float)e->targetX,(float)e->targetY);
    if(kind==EMSCRIPTEN_EVENT_MOUSEDOWN || kind==EMSCRIPTEN_EVENT_MOUSEUP) {
        int button=e->button==0?0:e->button==2?1:e->button==1?2:-1;
        if(button>=0) io.AddMouseButtonEvent(button,kind==EMSCRIPTEN_EVENT_MOUSEDOWN);
    }
    return io.WantCaptureMouse;
}
EM_BOOL wheelcb(int,const EmscriptenWheelEvent* e,void*) {
    float scale=e->deltaMode==DOM_DELTA_PIXEL?.01f:1.f;
    ImGui::GetIO().AddMouseWheelEvent((float)-e->deltaX*scale,(float)-e->deltaY*scale);
    return EM_TRUE;
}
EM_BOOL touchcb(int kind,const EmscriptenTouchEvent* e,void*) {
    for(int i=0;i<e->numTouches;++i) if(e->touches[i].isChanged) {
        auto& io=ImGui::GetIO();
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
        io.AddMousePosEvent((float)e->touches[i].targetX,(float)e->touches[i].targetY);
        io.AddMouseButtonEvent(0,kind!=EMSCRIPTEN_EVENT_TOUCHEND&&kind!=EMSCRIPTEN_EVENT_TOUCHCANCEL);
        break;
    }
    return EM_TRUE;
}
void frame() {
    double w=0,h=0;
    emscripten_get_element_css_size("#canvas",&w,&h);
    double dpr=std::max(1.0,emscripten_get_device_pixel_ratio());
    int fw=std::max(1,(int)(w*dpr)),fh=std::max(1,(int)(h*dpr));
    int cw=0,ch=0;
    emscripten_get_canvas_element_size("#canvas",&cw,&ch);
    if(fw!=cw||fh!=ch) emscripten_set_canvas_element_size("#canvas",fw,fh);
    auto& io=ImGui::GetIO();
    io.DisplaySize=ImVec2((float)w,(float)h);
    io.DisplayFramebufferScale=ImVec2((float)(fw/std::max(w,1.)),(float)(fh/std::max(h,1.)));
    double now=emscripten_get_now();
    io.DeltaTime=previous?std::max(.001f,(float)((now-previous)/1000.)):1.f/60.f;
    previous=now;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame(); app::draw(); ImGui::Render();
    glViewport(0,0,fw,fh);glClearColor(.065f,.075f,.105f,1);glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
}
int main() {
    EmscriptenWebGLContextAttributes attrs; emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion=2;attrs.alpha=EM_FALSE;
    auto ctx=emscripten_webgl_create_context("#canvas",&attrs);
    if(ctx<=0||emscripten_webgl_make_context_current(ctx)!=EMSCRIPTEN_RESULT_SUCCESS) return 1;
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGui::GetIO().BackendPlatformName="imgui_direct_web";
    app::setup();
    if(!ImGui_ImplOpenGL3_Init("#version 300 es")) return 2;
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,keycb);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,keycb);
    emscripten_set_mousedown_callback("#canvas",nullptr,true,mousecb);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,mousecb);
    emscripten_set_mousemove_callback("#canvas",nullptr,true,mousecb);
    emscripten_set_wheel_callback("#canvas",nullptr,true,wheelcb);
    emscripten_set_touchstart_callback("#canvas",nullptr,true,touchcb);
    emscripten_set_touchmove_callback("#canvas",nullptr,true,touchcb);
    emscripten_set_touchend_callback("#canvas",nullptr,true,touchcb);
    emscripten_set_touchcancel_callback("#canvas",nullptr,true,touchcb);
    emscripten_set_main_loop(frame,0,true);
}
