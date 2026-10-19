#include "app.hpp"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif
namespace {
EGLDisplay display=EGL_NO_DISPLAY;
EGLSurface surface=EGL_NO_SURFACE;
EGLContext context=EGL_NO_CONTEXT;
bool ready=false,focused=true;
void shutdown() {
    if(ready) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext(); ready=false;
    }
    if(display!=EGL_NO_DISPLAY) {
        eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        if(context!=EGL_NO_CONTEXT) eglDestroyContext(display,context);
        if(surface!=EGL_NO_SURFACE) eglDestroySurface(display,surface);
        eglTerminate(display);
    }
    display=EGL_NO_DISPLAY;surface=EGL_NO_SURFACE;context=EGL_NO_CONTEXT;
}
bool startup(ANativeWindow* window) {
    display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr))return false;
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
    EGLConfig config;EGLint count=0;
    if(!eglChooseConfig(display,attrs,&config,1,&count)||!count) return false;
    EGLint visual=0;eglGetConfigAttrib(display,config,EGL_NATIVE_VISUAL_ID,&visual);
    ANativeWindow_setBuffersGeometry(window,0,0,visual);
    EGLint cattrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    context=eglCreateContext(display,config,EGL_NO_CONTEXT,cattrs);
    surface=eglCreateWindowSurface(display,config,window,nullptr);
    if(context==EGL_NO_CONTEXT||surface==EGL_NO_SURFACE||
       !eglMakeCurrent(display,surface,surface,context))return false;
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); app::setup();
    ImGui::GetStyle().ScaleAllSizes(1.7f);
    ImGui::GetStyle().FontScaleDpi=1.7f;
    if(!ImGui_ImplAndroid_Init(window)||!ImGui_ImplOpenGL3_Init("#version 300 es"))return false;
    ready=true;return true;
}
void command(android_app* a,int32_t c) {
    if(c==APP_CMD_INIT_WINDOW&&a->window&&!ready&&!startup(a->window)) {
        __android_log_print(ANDROID_LOG_ERROR,"ImGuiDirect","Initialization failed");
        shutdown();
    } else if(c==APP_CMD_TERM_WINDOW)shutdown();
    else if(c==APP_CMD_GAINED_FOCUS)focused=true;
    else if(c==APP_CMD_LOST_FOCUS)focused=false;
}
int32_t input(android_app*,AInputEvent* e) {return ready?ImGui_ImplAndroid_HandleInputEvent(e):0;}
void frame(){
    ImGui_ImplOpenGL3_NewFrame();ImGui_ImplAndroid_NewFrame();ImGui::NewFrame();
    app::draw();ImGui::Render();
    auto& io=ImGui::GetIO();
    glViewport(0,0,(int)(io.DisplaySize.x*io.DisplayFramebufferScale.x),
                    (int)(io.DisplaySize.y*io.DisplayFramebufferScale.y));
    glClearColor(.065f,.075f,.105f,1);glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if(!eglSwapBuffers(display,surface))shutdown();
}
}
extern "C" void android_main(android_app* a){
    app_dummy();a->onAppCmd=command;a->onInputEvent=input;
    while(!a->destroyRequested){
        int events=0;android_poll_source* source=nullptr;
        while(ALooper_pollOnce(ready&&focused?0:-1,nullptr,&events,(void**)&source)>=0){
            if(source)source->process(a,source);
            if(a->destroyRequested||ready&&focused)break;
        }
        if(ready&&focused&&!a->destroyRequested)frame();
    }
    shutdown();
}
