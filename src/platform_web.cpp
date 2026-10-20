#include "app.hpp"
#include "ui/theme.hpp"
#include "ui/scroll.hpp"
#include "touch_imgui.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cfloat>
namespace {
double previous=0;
double lastTouch=-10000.;
bool isTouchDevice=false;
touch::Controller gestures;
// One browser-native input node is only an IME bridge, never an HTML UI.
EM_JS(void, updateNativeTextRect,(float x,float y,float width,float height,int visible),{
    const e=document.getElementById('native-input');
    if(!e)return;
    e.style.display=visible?'block':'none';
    if(visible){
        e.style.left=x+'px'; e.style.top=y+'px';
        e.style.width=width+'px'; e.style.height=height+'px';
    } else if(document.activeElement===e) e.blur();
});

EM_JS(void, report_scroll, (float x,float y,int rows,float pending,float mx,float my,float maxY,float posY,int inside,int packets), {
    window.__imguiScroll={appliedX:x, appliedY:y, rowsRendered:rows, time:performance.now()};
    window.__imguiScrollTotal=(window.__imguiScrollTotal||0)+y;
    window.__imguiScrollDebug={pending,mx,my,maxY,posY,inside,packets};
});
EM_JS(void, diagnostic_wheel, (float x,float y,float mouseX,float mouseY,int mode), {
    window.__imguiRawWheel={x,y,mouseX,mouseY,mode,received:(window.__imguiRawWheel?.received||0)+1};
});

EM_JS(void, report_pointer_source, (int source,int cursor,float x,float y), {
    window.__imguiPointerState={source,cursor,x,y};
});
EM_JS(void, update_canvas_cursor, (int hand), {
    const canvas=document.getElementById('canvas');
    if(!canvas)return;
    const cursor=hand?'pointer':'default';
    if(canvas.style.cursor!==cursor)canvas.style.cursor=cursor;
});
EM_JS(void, bootstrap_frame_ready, (), {
    const status=document.getElementById('status');
    if(status && status.textContent)status.textContent='';
});

EM_JS(void, browser_qa_state, (int page,int clicks), {
    window.__imguiUIState={page,clicks};
});

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
    if(emscripten_get_now()-lastTouch<700.)return EM_TRUE; // suppress synthesized touch mouse events
    auto& io=ImGui::GetIO();
    // Hybrid iPad/desktop: the last finger event must not make a real
    // trackpad/mouse hover look like a touch forever.
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    io.AddMousePosEvent((float)e->targetX,(float)e->targetY);
    if(kind==EMSCRIPTEN_EVENT_MOUSEDOWN || kind==EMSCRIPTEN_EVENT_MOUSEUP) {
        int button=e->button==0?0:e->button==2?1:e->button==1?2:-1;
        if(button>=0) io.AddMouseButtonEvent(button,kind==EMSCRIPTEN_EVENT_MOUSEDOWN);
    }
    return io.WantCaptureMouse;
}
EM_BOOL wheelcb(int,const EmscriptenWheelEvent* e,void*) {
    // DOM deltas are pixels, lines or pages. Deliver precise pixel events
    // 1:1. Chrome/macOS already send momentum in subsequent wheel events:
    // smoothing or adding another inertia engine here creates lag.
    const auto unit=e->deltaMode==DOM_DELTA_LINE?ui::scroll::Unit::Line:
                    e->deltaMode==DOM_DELTA_PAGE?ui::scroll::Unit::Page:
                    ui::scroll::Unit::Pixel;
    diagnostic_wheel((float)e->deltaX,(float)e->deltaY,
                     (float)e->mouse.targetX,(float)e->mouse.targetY,e->deltaMode);
    const float page=std::max(200.f,ImGui::GetIO().DisplaySize.y*.90f);
    if(e->mouse.ctrlKey) {
        // Browser trackpad pinch is exposed as Ctrl+wheel.
        app::zoom(std::exp(-(float)e->deltaY*.001f));
    } else {
        ImGui::GetIO().AddMouseSourceEvent(ImGuiMouseSource_Mouse);
        ImGui::GetIO().AddMousePosEvent((float)e->mouse.targetX,(float)e->mouse.targetY);
        ui::scroll::add_wheel((float)e->deltaX,(float)e->deltaY,unit,20.f,page);
    }
    return EM_TRUE;
}
EM_BOOL touchcb(int kind,const EmscriptenTouchEvent* e,void*) {
    lastTouch=emscripten_get_now();
    if(kind==EMSCRIPTEN_EVENT_TOUCHCANCEL){
        touch::dispatch(gestures.cancel());
        ui::scroll::state().discard();
        return EM_TRUE;
    }
    std::vector<touch::Contact> current;
    for(int i=0;i<e->numTouches;++i) {
        const auto& p=e->touches[i];
        if((kind==EMSCRIPTEN_EVENT_TOUCHEND||kind==EMSCRIPTEN_EVENT_TOUCHCANCEL)&&p.isChanged)continue;
        current.push_back({p.identifier,(float)p.targetX,(float)p.targetY,false});
    }
    std::sort(current.begin(),current.end(),[](const auto&a,const auto&b){return a.id<b.id;});
    touch::dispatch(gestures.update(current,emscripten_get_now()/1000.,!ui::scroll::drag_control_active()));
    return EM_TRUE;
}
EM_BOOL focuscb(int kind,const EmscriptenFocusEvent*,void*) {
    const bool focused=kind==EMSCRIPTEN_EVENT_FOCUS;
    if(!focused){
        touch::dispatch(gestures.cancel());
        ui::scroll::state().discard();
        lastTouch=emscripten_get_now();
        auto& io=ImGui::GetIO();
        io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
        for(int i=0;i<3;++i)io.AddMouseButtonEvent(i,false);
    }
    ImGui::GetIO().AddFocusEvent(focused);
    return EM_FALSE;
}
// Take both logical CSS size and the TRUE WebGL drawing-buffer size from the
// same browser snapshot. ResizeObserver owns canvas.width/height, preventing
// a competing C++ resize and a one-frame/stretch mismatch.
EM_JS(int, read_canvas_pixels, (double* outCssW, double* outCssH,
                               int* outWidth, int* outHeight,
                               int* outGlWidth, int* outGlHeight), {
    const adapter = window.__imguiCanvasPixels;
    if (!adapter) return 0;
    const state = adapter.sync();
    const gl = Module.canvas.getContext('webgl2');
    if (!state.ready || !gl) return 0;
    HEAPF64[outCssW >> 3] = state.cssWidth;
    HEAPF64[outCssH >> 3] = state.cssHeight;
    HEAP32[outWidth >> 2] = state.pixelWidth;
    HEAP32[outHeight >> 2] = state.pixelHeight;
    HEAP32[outGlWidth >> 2] = gl.drawingBufferWidth;
    HEAP32[outGlHeight >> 2] = gl.drawingBufferHeight;
    return 1;
});
void frame() {
    double w=0,h=0;
    int requestedW=0,requestedH=0,fw=0,fh=0;
    if (!read_canvas_pixels(&w,&h,&requestedW,&requestedH,&fw,&fh))
        return;
    // Rendering a different-size buffer would force the browser to resample
    // the completed picture, including text. Never render such a frame.
    if (requestedW!=fw || requestedH!=fh || fw<1 || fh<1)
        return;
    auto& io=ImGui::GetIO();
    io.DisplaySize=ImVec2((float)w,(float)h);
    io.DisplayFramebufferScale=ImVec2((float)((double)fw/w),(float)((double)fh/h));
    double now=emscripten_get_now();
    io.DeltaTime=previous?std::max(.001f,(float)((now-previous)/1000.)):1.f/60.f;
    previous=now;
    touch::dispatch(gestures.tick(now/1000.));
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame(); app::draw(); ImGui::Render();
    // Observable in browser QA; no DOM UI and no per-event allocations.
    report_scroll(ui::scroll::state().appliedX, ui::scroll::state().appliedY,
                  ui::scroll::state().rowsDrawn, ui::scroll::state().dy,
                  ui::scroll::state().lastPointerX,ui::scroll::state().lastPointerY,
                  ui::scroll::state().lastMaxY,ui::scroll::state().lastScrollY,
                  ui::scroll::state().lastInside?1:0,
                  (int)ui::scroll::state().packets);
    const auto background=ui::palette().base100;
    glViewport(0,0,fw,fh);
    glClearColor(background.r,background.g,background.b,1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // A touch-capable iPad can still have a real mouse/trackpad attached.
    // Follow the CURRENT pointer source, never only the device capability.
    report_pointer_source((int)ImGui::GetIO().MouseSource,
                          (int)ImGui::GetMouseCursor(),
                          ImGui::GetIO().MousePos.x,ImGui::GetIO().MousePos.y);
    update_canvas_cursor(
        ImGui::GetIO().MouseSource!=ImGuiMouseSource_TouchScreen &&
        ImGui::GetMouseCursor()==ImGuiMouseCursor_Hand);
    bootstrap_frame_ready();
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
    isTouchDevice=emscripten_run_script_int("('ontouchstart' in window) || navigator.maxTouchPoints>0");
    app::set_touch(isTouchDevice);
    if(isTouchDevice) ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_IsTouchScreen;
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
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,focuscb);
    emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,nullptr,true,focuscb);
    emscripten_set_main_loop(frame,0,true);
}

extern "C" EMSCRIPTEN_KEEPALIVE void mobile_insert_utf8(const char* value) {
    if(value)ImGui::GetIO().AddInputCharactersUTF8(value);
}
extern "C" EMSCRIPTEN_KEEPALIVE void mobile_backspace() {
    auto& io=ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_Backspace,true);
    io.AddKeyEvent(ImGuiKey_Backspace,false);
}
extern "C" EMSCRIPTEN_KEEPALIVE void mobile_focus() {app::request_text_focus();}
extern "C" void browser_input_rect(float x,float y,float width,float height) {
    updateNativeTextRect(x,y,width,height,isTouchDevice?1:0);
}
