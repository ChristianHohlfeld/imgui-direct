#include "app.hpp"
#include "touch_imgui.hpp"
#include "ui/scroll.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>
#include <cfloat>
// Dear ImGui: standalone example application for OSX + Metal.

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#import <Foundation/Foundation.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#endif

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "imgui.h"
#include "imgui_impl_metal.h"
#if TARGET_OS_OSX
#include "imgui_impl_osx.h"
@interface AppViewController : NSViewController<NSWindowDelegate>
@end
#else
@interface AppViewController : UIViewController
@end
#endif

#if !TARGET_OS_OSX
// Native UIKit IME bridge. The application UI still belongs entirely to ImGui/C++.
@interface DirectTextView : UITextView
@end
@implementation DirectTextView
- (void)deleteBackward {
    if (self.text.length == 0 && !self.markedTextRange) {
        auto& io=ImGui::GetIO();
        io.AddKeyEvent(ImGuiKey_Backspace,true);
        io.AddKeyEvent(ImGuiKey_Backspace,false);
    } else {
        [super deleteBackward];
    }
}
@end
static std::string gClipboard;
static const char* GetClipboard(ImGuiContext*) {
    NSString* value=[UIPasteboard generalPasteboard].string;
    if(!value)return nullptr;
    gClipboard=value.UTF8String ?: "";
    return gClipboard.c_str();
}
static void SetClipboard(ImGuiContext*,const char* value) {
    [UIPasteboard generalPasteboard].string=[NSString stringWithUTF8String:value ? value : ""];
}
static AppViewController* gController=nil;
static void SetIme(ImGuiContext*,ImGuiViewport*,ImGuiPlatformImeData* data);
#endif

#if TARGET_OS_OSX
@interface AppViewController () <MTKViewDelegate>
@property (nonatomic, strong) id scrollMonitor;
#else
@interface AppViewController () <MTKViewDelegate, UITextViewDelegate>
@property (nonatomic, strong) DirectTextView *keyboardSink;
- (void)syncKeyboard:(BOOL)shouldShow;
- (void)keyboardFrameChanged:(NSNotification *)notification;
- (void)onHover:(UIHoverGestureRecognizer*)recognizer;
- (void)onIndirectScroll:(UIPanGestureRecognizer*)recognizer;
#endif
@property (nonatomic, readonly) MTKView *mtkView;
@property (nonatomic, strong) id <MTLDevice> device;
@property (nonatomic, strong) id <MTLCommandQueue> commandQueue;
@end

//-----------------------------------------------------------------------------------
// AppViewController
//-----------------------------------------------------------------------------------

@implementation AppViewController
#if !TARGET_OS_OSX
{
    touch::Controller _touchController;
}
#endif

-(instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];

    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];

    if (!self.device)
    {
        NSLog(@"Metal is not supported");
        abort();
    }

    // Setup Dear ImGui context
    // FIXME: This example doesn't have proper cleanup...
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    app::setup();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Native theme is installed by app::setup(); don't overwrite it here.

    // Setup Renderer backend
    ImGui_ImplMetal_Init(_device);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    return self;
}

-(MTKView *)mtkView
{
    return (MTKView *)self.view;
}

-(void)loadView
{
    self.view = [[MTKView alloc] initWithFrame:CGRectMake(0, 0, 1200, 800)];
}

-(void)viewDidLoad
{
    [super viewDidLoad];

    self.mtkView.device = self.device;
    self.mtkView.delegate = self;

#if TARGET_OS_OSX
    ImGui_ImplOSX_Init(self.view);
    // AppKit already generates momentum-phase scroll events with its native
    // acceleration. Forward scrollingDelta directly in logical points and
    // consume the event so the ImGui OSX backend cannot rescale it again.
    AppViewController* __unsafe_unretained weakSelf=self;
    self.scrollMonitor=[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
        handler:^NSEvent*(NSEvent* event) {
            AppViewController* controller=weakSelf;
            if(!controller || event.window!=controller.view.window)return event;
            NSPoint local=[controller.view convertPoint:event.locationInWindow fromView:nil];
            if(!NSPointInRect(local,controller.view.bounds))return event;
            if(event.phase==NSEventPhaseCancelled)return nil;
            const float step=event.hasPreciseScrollingDeltas?1.f:20.f;
            ui::scroll::add((float)-event.scrollingDeltaX*step,
                            (float)-event.scrollingDeltaY*step);
            return nil;
        }];
    [NSApp activateIgnoringOtherApps:YES];
#else
    gController=self;
    app::set_touch(true);
    auto& platform=ImGui::GetPlatformIO();
    platform.Platform_GetClipboardTextFn=GetClipboard;
    platform.Platform_SetClipboardTextFn=SetClipboard;
    platform.Platform_SetImeDataFn=SetIme;

    self.keyboardSink=[[DirectTextView alloc] initWithFrame:CGRectMake(1,1,2,2)];
    self.keyboardSink.delegate=self;
    self.keyboardSink.backgroundColor=[UIColor clearColor];
    self.keyboardSink.alpha=.01;
    self.keyboardSink.autocapitalizationType=UITextAutocapitalizationTypeNone;
    self.keyboardSink.autocorrectionType=UITextAutocorrectionTypeNo;
    self.keyboardSink.spellCheckingType=UITextSpellCheckingTypeNo;
    self.keyboardSink.keyboardType=UIKeyboardTypeDefault;
    self.keyboardSink.returnKeyType=UIReturnKeyDone;
    self.keyboardSink.scrollEnabled=NO;
    [self.view addSubview:self.keyboardSink];

    UIHoverGestureRecognizer* hover=[[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(onHover:)];
    [self.view addGestureRecognizer:hover];

    UIPanGestureRecognizer* indirect=[[UIPanGestureRecognizer alloc]
        initWithTarget:self action:@selector(onIndirectScroll:)];
    indirect.allowedScrollTypesMask=UIScrollTypeMaskAll;
    indirect.allowedTouchTypes=@[]; // Only trackpad/wheel, never steal finger gestures.
    indirect.cancelsTouchesInView=NO;
    [self.view addGestureRecognizer:indirect];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardFrameChanged:)
        name:UIKeyboardWillChangeFrameNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardFrameChanged:)
        name:UIKeyboardWillHideNotification object:nil];
#endif
}

-(void)drawInMTKView:(MTKView*)view
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = view.bounds.size.width;
    io.DisplaySize.y = view.bounds.size.height;

#if TARGET_OS_OSX
    CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
#else
    CGFloat framebufferScale = view.window.screen.scale ?: UIScreen.mainScreen.scale;
#endif
    io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);
#if !TARGET_OS_OSX
    UIEdgeInsets safe=view.safeAreaInsets;
    app::safe_area((float)safe.top,(float)safe.right,(float)safe.bottom,(float)safe.left);
    touch::dispatch(_touchController.tick(CACurrentMediaTime()));
#endif

    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];

    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
    if (renderPassDescriptor == nil)
    {
        [commandBuffer commit];
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
#if TARGET_OS_OSX
    ImGui_ImplOSX_NewFrame(view);
#endif
    ImGui::NewFrame();
    app::draw();

    // Our state (make them static = more or less global) as a convenience to keep the example terse.
    static bool show_demo_window = true;
    static bool show_another_window = false;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImGui_ImplMetal_RenderDrawData(draw_data, commandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];

    // Present
    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
}

-(void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size
{
}

//-----------------------------------------------------------------------------------
// Input processing
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

- (void)dealloc {
    if(self.scrollMonitor)[NSEvent removeMonitor:self.scrollMonitor];
}

- (void)viewWillAppear
{
    [super viewWillAppear];
    self.view.window.delegate = self;
}

- (void)windowWillClose:(NSNotification *)notification
{
    if(self.scrollMonitor){[NSEvent removeMonitor:self.scrollMonitor];self.scrollMonitor=nil;}
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext();
}

#else

// Touch points are identified by UIKit's stable UITouch object identity.
// Pencil remains a pen source; mouse/trackpad events are handled separately.
-(void)updateIOWithTouchEvent:(UIEvent*)event
{
    std::vector<touch::Contact> points;
    for(UITouch* t in event.allTouches) {
        if(t.type==UITouchTypeIndirectPointer) {
            CGPoint p=[t locationInView:self.view];
            auto& io=ImGui::GetIO();
            io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
            io.AddMousePosEvent((float)p.x,(float)p.y);
            int button=(event.buttonMask&UIEventButtonMaskSecondary)?1:0;
            io.AddMouseButtonEvent(button,
                t.phase!=UITouchPhaseEnded&&t.phase!=UITouchPhaseCancelled);
            continue;
        }
        if(t.phase==UITouchPhaseEnded||t.phase==UITouchPhaseCancelled) continue;
        CGPoint p=[t locationInView:self.view];
        points.push_back({(std::int64_t)(std::uintptr_t)(__bridge void*)t,
            (float)p.x,(float)p.y,t.type==UITouchTypePencil});
    }
    std::sort(points.begin(),points.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    touch::dispatch(_touchController.update(points,CACurrentMediaTime(),!ImGui::IsAnyItemActive()));
}
-(void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {[self updateIOWithTouchEvent:event];}
-(void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {[self updateIOWithTouchEvent:event];}
-(void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {[self updateIOWithTouchEvent:event];}
-(void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {[self updateIOWithTouchEvent:event];}

-(void)onHover:(UIHoverGestureRecognizer*)recognizer {
    auto& io=ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    if(recognizer.state==UIGestureRecognizerStateEnded || recognizer.state==UIGestureRecognizerStateCancelled)
        io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
    else {
        CGPoint p=[recognizer locationInView:self.view];
        io.AddMousePosEvent((float)p.x,(float)p.y);
    }
}
-(void)onIndirectScroll:(UIPanGestureRecognizer*)recognizer {
    if(recognizer.state==UIGestureRecognizerStateBegan ||
       recognizer.state==UIGestureRecognizerStateChanged) {
        CGPoint translation=[recognizer translationInView:self.view];
        if(translation.x!=0 || translation.y!=0) {
            ui::scroll::add((float)-translation.x,(float)-translation.y);
            [recognizer setTranslation:CGPointZero inView:self.view];
        }
    }
}
-(void)syncKeyboard:(BOOL)show {
    if(show && !self.keyboardSink.isFirstResponder)[self.keyboardSink becomeFirstResponder];
    else if(!show && self.keyboardSink.isFirstResponder)[self.keyboardSink resignFirstResponder];
}
-(void)textViewDidChange:(UITextView*)view {
    // Keep marked composition in UIKit until committed (accent/CJK/emoji).
    if(view.markedTextRange)return;
    NSString* text=view.text;
    if(text.length) {
        ImGui::GetIO().AddInputCharactersUTF8(text.UTF8String);
        view.text=@"";
    }
}
-(void)keyboardFrameChanged:(NSNotification*)note {
    CGRect r=[note.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect windowRect=[self.view convertRect:self.view.bounds toView:nil];
    CGFloat intersection=CGRectGetHeight(CGRectIntersection(r,windowRect));
    if([note.name isEqualToString:UIKeyboardWillHideNotification])intersection=0;
    app::keyboard_height((float)std::max(0.0,(double)intersection));
}
-(void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    _touchController.reset();
    ImGui::GetIO().AddFocusEvent(false);
    [self.keyboardSink resignFirstResponder];
}
-(void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    ImGui::GetIO().AddFocusEvent(true);
}
-(void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if(gController==self)gController=nil;
}

#endif

@end

#if !TARGET_OS_OSX
static void SetIme(ImGuiContext*,ImGuiViewport*,ImGuiPlatformImeData* data)
{
    if(gController)[gController syncKeyboard:data->WantTextInput];
}
#endif

//-----------------------------------------------------------------------------------
// AppDelegate
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@end

@implementation AppDelegate

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

-(instancetype)init
{
    if (self = [super init])
    {
        NSViewController *rootViewController = [[AppViewController alloc] initWithNibName:nil bundle:nil];
        self.window = [[NSWindow alloc] initWithContentRect:NSZeroRect
                                                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
        self.window.contentViewController = rootViewController;
        [self.window center];
        [self.window makeKeyAndOrderFront:self];
    }
    return self;
}

@end

#else

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation AppDelegate

-(BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey,id> *)launchOptions
{
    UIViewController *rootViewController = [[AppViewController alloc] init];
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = rootViewController;
    [self.window makeKeyAndVisible];
    return YES;
}

@end

#endif

//-----------------------------------------------------------------------------------
// Application main() function
//-----------------------------------------------------------------------------------

#if TARGET_OS_OSX

int main(int, const char**)
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        AppDelegate *appDelegate = [[AppDelegate alloc] init];   // creates window
        [NSApp setDelegate:appDelegate];

        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

#else

int main(int argc, char * argv[])
{
    @autoreleasepool
    {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#endif
