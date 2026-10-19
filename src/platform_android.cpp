#include "touch_imgui.hpp"
#include "ui/scroll.hpp"
#include "ui/theme.hpp"
#include <android/configuration.h>
#include "utf.hpp"
#include <android/input.h>
#include <jni.h>
#include <mutex>
#include <deque>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
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
touch::Controller gestures;
android_app* gApp=nullptr;
std::mutex textMutex;
struct InputMessage {bool backspace;std::string text;};
std::deque<InputMessage> pendingInput;
std::string clipboardText;
std::string fromJString(JNIEnv* env,jstring text) {
    if(!text)return {};
    const jsize n=env->GetStringLength(text);
    const jchar* data=env->GetStringChars(text,nullptr);
    if(!data)return {};
    const std::u16string_view value((const char16_t*)data,(size_t)n);
    std::string result=unicode::utf16_to_utf8(value);
    env->ReleaseStringChars(text,data);
    return result;
}
jstring toJString(JNIEnv* env,const char* text) {
    std::u16string value=unicode::utf8_to_utf16(text?text:"");
    return env->NewString((const jchar*)value.data(),(jsize)value.size());
}
bool keyboardVisible=false;
double clockSeconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}
// Android's platform text/clipboard integration is thin JNI glue, not another UI.
template<class Func> void withJava(Func f) {
    if(!gApp||!gApp->activity)return;
    JavaVM* vm=gApp->activity->vm;
    JNIEnv* env=nullptr;
    bool attached=false;
    if(vm->GetEnv((void**)&env,JNI_VERSION_1_6)==JNI_EDETACHED) {
        if(vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return;
        attached=true;
    }
    f(env,gApp->activity->clazz);
    if(env->ExceptionCheck()) {env->ExceptionDescribe();env->ExceptionClear();}
    if(attached)vm->DetachCurrentThread();
}
void callKeyboard(bool show) {
    withJava([&](JNIEnv* e,jobject act) {
        jclass cls=e->GetObjectClass(act);
        jmethodID method=e->GetMethodID(cls,"showKeyboard","(Z)V");
        if(method)e->CallVoidMethod(act,method,(jboolean)show);
        e->DeleteLocalRef(cls);
    });
}
const char* getClipboard(ImGuiContext*) {
    withJava([&](JNIEnv* e,jobject act) {
        jclass cls=e->GetObjectClass(act);
        jmethodID m=e->GetMethodID(cls,"getClipboardText","()Ljava/lang/String;");
        if(m) {
            jstring text=(jstring)e->CallObjectMethod(act,m);
            if(text) {
                clipboardText=fromJString(e,text);
                e->DeleteLocalRef(text);
            } else clipboardText.clear();
        }
        e->DeleteLocalRef(cls);
    });
    return clipboardText.c_str();
}
void setClipboard(ImGuiContext*,const char* value) {
    withJava([&](JNIEnv* e,jobject act) {
        jclass cls=e->GetObjectClass(act);
        jmethodID m=e->GetMethodID(cls,"setClipboardText","(Ljava/lang/String;)V");
        if(m) {
            jstring text=toJString(e,value);
            e->CallVoidMethod(act,m,text);
            e->DeleteLocalRef(text);
        }
        e->DeleteLocalRef(cls);
    });
}
void drainText() {
    std::deque<InputMessage> msgs;
    { std::lock_guard<std::mutex> lock(textMutex); msgs.swap(pendingInput); }
    auto& io=ImGui::GetIO();
    for(const auto& m:msgs) {
        if(m.backspace) {
            io.AddKeyEvent(ImGuiKey_Backspace,true);
            io.AddKeyEvent(ImGuiKey_Backspace,false);
        } else io.AddInputCharactersUTF8(m.text.c_str());
    }
}

void shutdown() {
    if(ready) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext(); ready=false;
        gestures.reset(); keyboardVisible=false;
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
    app::set_touch(true);
    ImGui::GetPlatformIO().Platform_GetClipboardTextFn=getClipboard;
    ImGui::GetPlatformIO().Platform_SetClipboardTextFn=setClipboard;
    // ImGui uses physical surface pixels here, unlike UIKit CSS/logical points.
    // Calculate density once, and let the shared theme system set font + hit sizes.
    int dpi=gApp && gApp->config ? AConfiguration_getDensity(gApp->config) : 160;
    float density=(dpi>100 && dpi<1000)?float(dpi)/160.f:1.f;
    ui::set_scale(density);
    if(!ImGui_ImplAndroid_Init(window)||!ImGui_ImplOpenGL3_Init("#version 300 es"))return false;
    ready=true;return true;
}
void command(android_app* a,int32_t c) {
    if(c==APP_CMD_INIT_WINDOW&&a->window&&!ready&&!startup(a->window)) {
        __android_log_print(ANDROID_LOG_ERROR,"ImGuiDirect","Initialization failed");
        shutdown();
    } else if(c==APP_CMD_TERM_WINDOW)shutdown();
    else if(c==APP_CMD_GAINED_FOCUS){focused=true;if(ready)ImGui::GetIO().AddFocusEvent(true);}
    else if(c==APP_CMD_LOST_FOCUS){focused=false;gestures.reset();if(ready)ImGui::GetIO().AddFocusEvent(false);}
}
int32_t input(android_app*,AInputEvent* e) {
    if(!ready)return 0;
    if(AInputEvent_getType(e)==AINPUT_EVENT_TYPE_MOTION) {
        int source=AInputEvent_getSource(e);
        if((AMotionEvent_getAction(e)&AMOTION_EVENT_ACTION_MASK)==AMOTION_EVENT_ACTION_SCROLL) {
            const float hx=AMotionEvent_getAxisValue(e,AMOTION_EVENT_AXIS_HSCROLL,0);
            const float vy=AMotionEvent_getAxisValue(e,AMOTION_EVENT_AXIS_VSCROLL,0);
            ui::scroll::add(-hx*48.f,-vy*48.f);
            return 1;
        }
        if((source & AINPUT_SOURCE_TOUCHSCREEN)==AINPUT_SOURCE_TOUCHSCREEN) {
            const int action=AMotionEvent_getAction(e);
            const int kind=action&AMOTION_EVENT_ACTION_MASK;
            const int changed=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            std::vector<touch::Contact> active;
            if(kind!=AMOTION_EVENT_ACTION_CANCEL) {
                size_t count=AMotionEvent_getPointerCount(e);
                for(size_t i=0;i<count;++i) {
                    if((kind==AMOTION_EVENT_ACTION_UP||kind==AMOTION_EVENT_ACTION_POINTER_UP)&&
                       (int)i==changed)continue;
                    active.push_back({AMotionEvent_getPointerId(e,i),
                        AMotionEvent_getX(e,i),AMotionEvent_getY(e,i),false});
                }
                std::sort(active.begin(),active.end(),[](auto&a,auto&b){return a.id<b.id;});
            }
            touch::dispatch(gestures.update(active,clockSeconds(),!ui::scroll::drag_control_active()));
            return 1;
        }
    }
    // Physical keyboards, mice, trackpads and wheel input still use the official backend.
    return ImGui_ImplAndroid_HandleInputEvent(e);
}
void frame(){
    drainText();
    touch::dispatch(gestures.tick(clockSeconds()));
    ImGui_ImplOpenGL3_NewFrame();ImGui_ImplAndroid_NewFrame();ImGui::NewFrame();
    app::draw();ImGui::Render();
    bool wantsKeyboard=ImGui::GetIO().WantTextInput;
    if(wantsKeyboard!=keyboardVisible) {
        keyboardVisible=wantsKeyboard;
        callKeyboard(wantsKeyboard);
    }
    auto& io=ImGui::GetIO();
    glViewport(0,0,(int)(io.DisplaySize.x*io.DisplayFramebufferScale.x),
                    (int)(io.DisplaySize.y*io.DisplayFramebufferScale.y));
    glClearColor(.065f,.075f,.105f,1);glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if(!eglSwapBuffers(display,surface))shutdown();
}
}
extern "C" void android_main(android_app* a){
    app_dummy();gApp=a;a->onAppCmd=command;a->onInputEvent=input;
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

// JNI calls originate from the Android UI/IME thread. Never mutate ImGui from
// that thread; queue UTF-8 text and consume on the GL thread in frame().
extern "C" JNIEXPORT void JNICALL
Java_de_imgui_direct_ImGuiInputActivity_nativeText(JNIEnv* env,jclass,jstring jtext) {
    if(!jtext)return;
    auto text=fromJString(env,jtext);
    {std::lock_guard<std::mutex> lock(textMutex);pendingInput.push_back({false,std::move(text)});}
}
extern "C" JNIEXPORT void JNICALL
Java_de_imgui_direct_ImGuiInputActivity_nativeBackspace(JNIEnv*,jclass) {
    std::lock_guard<std::mutex> lock(textMutex);
    pendingInput.push_back({true,{}});
}
