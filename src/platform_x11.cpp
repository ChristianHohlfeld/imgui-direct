#include "app.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <chrono>
#include <cfloat>
#include <cstdio>
#ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#endif
namespace {
using ContextFactory=GLXContext(*)(Display*,GLXFBConfig,GLXContext,Bool,const int*);
ImGuiKey key(KeySym k){
    if(k>=XK_a&&k<=XK_z)return (ImGuiKey)(ImGuiKey_A+k-XK_a);
    if(k>=XK_A&&k<=XK_Z)return (ImGuiKey)(ImGuiKey_A+k-XK_A);
    if(k>=XK_0&&k<=XK_9)return (ImGuiKey)(ImGuiKey_0+k-XK_0);
    switch(k){
        case XK_Tab:return ImGuiKey_Tab;case XK_Left:return ImGuiKey_LeftArrow;
        case XK_Right:return ImGuiKey_RightArrow;case XK_Up:return ImGuiKey_UpArrow;
        case XK_Down:return ImGuiKey_DownArrow;case XK_BackSpace:return ImGuiKey_Backspace;
        case XK_Return:return ImGuiKey_Enter;case XK_Delete:return ImGuiKey_Delete;
        case XK_Escape:return ImGuiKey_Escape;case XK_space:return ImGuiKey_Space;
        case XK_Shift_L:return ImGuiKey_LeftShift;case XK_Shift_R:return ImGuiKey_RightShift;
        case XK_Control_L:return ImGuiKey_LeftCtrl;case XK_Control_R:return ImGuiKey_RightCtrl;
        case XK_Alt_L:return ImGuiKey_LeftAlt;case XK_Alt_R:return ImGuiKey_RightAlt;
        case XK_Home:return ImGuiKey_Home;case XK_End:return ImGuiKey_End;
        default:return ImGuiKey_None;
    }
}
void event(XEvent& e,Atom close,bool& run){
    auto& io=ImGui::GetIO();
    switch(e.type){
    case ClientMessage:if((Atom)e.xclient.data.l[0]==close)run=false;break;
    case DestroyNotify:run=false;break;
    case MotionNotify:io.AddMousePosEvent((float)e.xmotion.x,(float)e.xmotion.y);break;
    case LeaveNotify:io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);break;
    case ButtonPress:case ButtonRelease:{
        bool down=e.type==ButtonPress;
        if(e.xbutton.button==4&&down)io.AddMouseWheelEvent(0,1);
        if(e.xbutton.button==5&&down)io.AddMouseWheelEvent(0,-1);
        int b=e.xbutton.button==1?0:e.xbutton.button==3?1:e.xbutton.button==2?2:-1;
        if(b>=0)io.AddMouseButtonEvent(b,down);
        break;
    }
    case KeyPress:case KeyRelease:{
        bool down=e.type==KeyPress;
        KeySym ks=XLookupKeysym(&e.xkey,0);
        ImGuiKey k=key(ks);if(k!=ImGuiKey_None)io.AddKeyEvent(k,down);
        io.AddKeyEvent(ImGuiMod_Ctrl,!!(e.xkey.state&ControlMask));
        io.AddKeyEvent(ImGuiMod_Shift,!!(e.xkey.state&ShiftMask));
        io.AddKeyEvent(ImGuiMod_Alt,!!(e.xkey.state&Mod1Mask));
        if(down){
            char chars[64]{};
            KeySym unused;
            int n=XLookupString(&e.xkey,chars,sizeof(chars)-1,&unused,nullptr);
            if(n>0&&(unsigned char)chars[0]>=32&&!(e.xkey.state&ControlMask)){
                chars[n]=0;io.AddInputCharactersUTF8(chars);
            }
        }
        break;
    }
    }
}
}
int main(){
    Display* d=XOpenDisplay(nullptr);
    if(!d){std::fputs("X11 unavailable\n",stderr);return 1;}
    int a[]={GLX_X_RENDERABLE,True,GLX_DRAWABLE_TYPE,GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,GLX_RGBA_BIT,GLX_X_VISUAL_TYPE,GLX_TRUE_COLOR,
        GLX_RED_SIZE,8,GLX_GREEN_SIZE,8,GLX_BLUE_SIZE,8,GLX_DOUBLEBUFFER,True,None};
    int count=0;
    GLXFBConfig* fbs=glXChooseFBConfig(d,DefaultScreen(d),a,&count);
    if(!fbs||!count)return 2;
    GLXFBConfig fb=fbs[0];
    XVisualInfo* vi=glXGetVisualFromFBConfig(d,fb);
    XSetWindowAttributes wa{};
    wa.colormap=XCreateColormap(d,RootWindow(d,vi->screen),vi->visual,AllocNone);
    wa.event_mask=StructureNotifyMask|KeyPressMask|KeyReleaseMask|
                 ButtonPressMask|ButtonReleaseMask|PointerMotionMask|LeaveWindowMask;
    Window w=XCreateWindow(d,RootWindow(d,vi->screen),0,0,1100,720,0,
      vi->depth,InputOutput,vi->visual,CWColormap|CWEventMask,&wa);
    XStoreName(d,w,"ImGui Direct");Atom close=XInternAtom(d,"WM_DELETE_WINDOW",False);
    XSetWMProtocols(d,w,&close,1);XMapWindow(d,w);
    auto factory=(ContextFactory)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    int ctxattrs[]={GLX_CONTEXT_MAJOR_VERSION_ARB,3,GLX_CONTEXT_MINOR_VERSION_ARB,0,None};
    GLXContext gl=factory?factory(d,fb,nullptr,True,ctxattrs):nullptr;
    XFree(vi);XFree(fbs);
    if(!gl||!glXMakeCurrent(d,w,gl))return 3;
    IMGUI_CHECKVERSION();ImGui::CreateContext();app::setup();
    ImGui::GetIO().BackendPlatformName="imgui_direct_x11";
    if(!ImGui_ImplOpenGL3_Init("#version 130"))return 4;
    auto prev=std::chrono::steady_clock::now();bool running=true;
    while(running){
        while(XPending(d)){XEvent e;XNextEvent(d,&e);event(e,close,running);}
        if(!running)break;
        XWindowAttributes wa2;XGetWindowAttributes(d,w,&wa2);
        if(wa2.width<1||wa2.height<1)continue;
        auto now=std::chrono::steady_clock::now();
        float dt=std::chrono::duration<float>(now-prev).count();prev=now;
        auto& io=ImGui::GetIO();
        io.DisplaySize=ImVec2((float)wa2.width,(float)wa2.height);
        io.DeltaTime=dt>0?dt:1.f/60.f;
        ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();app::draw();ImGui::Render();
        glViewport(0,0,wa2.width,wa2.height);glClearColor(.065f,.075f,.105f,1);
        glClear(GL_COLOR_BUFFER_BIT);ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glXSwapBuffers(d,w);
    }
    ImGui_ImplOpenGL3_Shutdown();ImGui::DestroyContext();
    glXMakeCurrent(d,None,nullptr);glXDestroyContext(d,gl);
    XDestroyWindow(d,w);XFreeColormap(d,wa.colormap);XCloseDisplay(d);
    return 0;
}
