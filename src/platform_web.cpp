#include "app.hpp"
#include "state_gallery.hpp"
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
namespace {
double previous=0;
double lastTouch=-10000.;
bool isTouchDevice=false;
touch::Controller gestures;
// One browser-native input node is only an IME bridge, never an HTML UI.
EM_JS(void, browser_input_frame,(), {
    window.__imguiInputRegions=[];
});
EM_JS(void, browser_input_region,(float x,float y,float width,float height,int active), {
    if(width>0&&height>0)window.__imguiInputRegions.push({x,y,width,height,active:!!active});
});
EM_JS(int, native_text_focused,(), {
    return document.activeElement===document.getElementById('native-input')?1:0;
});

EM_JS(void, report_scroll, (float x,float y,int rows,float pending,float mx,float my,float maxY,float posY,int inside,int packets,float elastic), {
    window.__imguiScroll={appliedX:x, appliedY:y, rowsRendered:rows, time:performance.now()};
    window.__imguiScrollTotal=(window.__imguiScrollTotal||0)+y;
    window.__imguiScrollDebug={pending,mx,my,maxY,posY,inside,packets,elasticY:elastic};
});
EM_JS(void, diagnostic_wheel, (float x,float y,float mouseX,float mouseY,int mode), {
    window.__imguiRawWheel={x,y,mouseX,mouseY,mode,received:(window.__imguiRawWheel?.received||0)+1};
});

EM_JS(void, bootstrap_frame_ready, (), {
    const status=document.getElementById('status');
    if(status && status.textContent)status.textContent='';
});

// QA only: read the completed GPU frame before WebGL discards its back buffer.
// No work or allocation occurs unless the test runner requests an image.
EM_JS(void, browser_qa_capture_frame,(),{
    const request=window.__imguiCaptureRequest;
    if(!request)return;
    const canvas=Module.canvas,rect=canvas.getBoundingClientRect();
    const area=request.clip||{x:0,y:0,width:rect.width,height:rect.height};
    const sx=Math.max(0,Math.round(area.x*canvas.width/rect.width));
    const sy=Math.max(0,Math.round(area.y*canvas.height/rect.height));
    const sw=Math.max(1,Math.min(canvas.width-sx,Math.round(area.width*canvas.width/rect.width)));
    const sh=Math.max(1,Math.min(canvas.height-sy,Math.round(area.height*canvas.height/rect.height)));
    const image=window.__imguiCaptureCanvas||(window.__imguiCaptureCanvas=document.createElement('canvas'));
    image.width=sw;image.height=sh;
    image.getContext('2d').drawImage(canvas,sx,sy,sw,sh,0,0,sw,sh);
    window.__imguiCaptureResult={id:request.id,data:image.toDataURL('image/png'),width:sw,height:sh};
    window.__imguiCaptureRequest=null;
});

EM_JS(void, browser_qa_state, (int page,int clicks), {
    window.__imguiUIState={page,clicks};
});

EM_JS(void, browser_qa_gallery, (int matching,int drawn,int filter), {
    window.__imguiGallery={matching,drawn,filter};
});
EM_JS(void, browser_qa_item, (int id,float x,float y,float w,float h), {
    window.__imguiItems=window.__imguiItems||{};
    window.__imguiItems[id]={x,y,w,h};
});

EM_JS(void, browser_qa_field, (int id,const char* value), {
    window.__imguiFields=window.__imguiFields||{};
    window.__imguiFields[id]=UTF8ToString(value);
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
    // Native textarea owns text/composition events while IME is focused.
    // Feeding the same key through both paths would insert every character twice.
    if(native_text_focused())return EM_FALSE;
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
        ImGui::GetIO().AddMousePosEvent((float)e->mouse.targetX,(float)e->mouse.targetY);
        ui::scroll::add_wheel((float)e->deltaX,(float)e->deltaY,unit,20.f,page);
    }
    return EM_TRUE;
}
EM_BOOL touchcb(int kind,const EmscriptenTouchEvent* e,void*) {
    lastTouch=emscripten_get_now();
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
                  (int)ui::scroll::state().packets,ui::scroll::state().overscrollY);
    const auto background=ui::palette().base100;
    glViewport(0,0,fw,fh);
    glClearColor(background.r,background.g,background.b,1);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    bootstrap_frame_ready();
    browser_qa_capture_frame();
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
    // Respect accessibility: reduced-motion users get precise scrolling,
    // without any elastic displacement animation.
    ui::scroll::set_reduce_motion(emscripten_run_script_int(
        "window.matchMedia && matchMedia('(prefers-reduced-motion: reduce)').matches ? 1 : 0")!=0);
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

extern "C" EMSCRIPTEN_KEEPALIVE void mobile_control(int key) {
    ImGuiKey mapped=ImGuiKey_None;
    switch(key){case 13:mapped=ImGuiKey_Enter;break;case 9:mapped=ImGuiKey_Tab;break;
    case 27:mapped=ImGuiKey_Escape;break;case 37:mapped=ImGuiKey_LeftArrow;break;case 39:mapped=ImGuiKey_RightArrow;break;
    case 38:mapped=ImGuiKey_UpArrow;break;case 40:mapped=ImGuiKey_DownArrow;break;
    case 36:mapped=ImGuiKey_Home;break;case 35:mapped=ImGuiKey_End;break;case 46:mapped=ImGuiKey_Delete;break;}
    if(mapped!=ImGuiKey_None){auto& io=ImGui::GetIO();io.AddKeyEvent(mapped,true);io.AddKeyEvent(mapped,false);}
}

extern "C" EMSCRIPTEN_KEEPALIVE void gallery_review(int component,int state,int dark,int size){
    app::review_configure(component,state,dark,size);
}
EM_JS(void, browser_qa_review,(int component,int state,int changes,int hover,int active,int focused,int disabled,float x,float y,float w,float h,float px,float py,float pw,float ph),{
    window.__imguiReview={time:performance.now(),component,state,changes,hover:!!hover,active:!!active,focused:!!focused,disabled:!!disabled,rect:{x,y,w,h},preview:{x:px,y:py,w:pw,h:ph}};
});

EM_JS(void, browser_qa_catalog,(int id,const char* name,unsigned states,int interactive),{
    window.__imguiReviewCatalog=window.__imguiReviewCatalog||[];
    window.__imguiReviewCatalog[id]={id,name:UTF8ToString(name),states,interactive:!!interactive};
});

extern "C" EMSCRIPTEN_KEEPALIVE void gallery_review_variant(int variant){
    review::set_variant(static_cast<ui::Variant>(std::clamp(variant,0,9)));
}
