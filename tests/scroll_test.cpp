#include "ui/scroll.hpp"
#include <cassert>
#include <cmath>
int main(){
    using namespace ui::scroll;
    assert(to_pixels(12.5f,Unit::Pixel)==12.5f);
    assert(to_pixels(3.f,Unit::Line,20.f)==60.f);
    assert(to_pixels(1.f,Unit::Page,20.f,700.f)==700.f);
    assert(to_pixels(-1.25f,Unit::Pixel)==-1.25f);
    assert(to_pixels(NAN,Unit::Pixel)==0.f);
    Buffer b;
    b.add(0.f,1.25f);b.add(0.f,1.75f);
    assert(b.dy==3.f);
    b.next_frame();assert(b.dy==3.f&&b.appliedY==0);
    b.discard();assert(b.dy==0);
    b.push_edge(-100.f);
    assert(b.overscrollY<0.f&&b.overscrollY>=-56.f);
    b.push_edge(-3000.f);
    assert(b.overscrollY==-56.f);
    for(int i=0;i<120;++i)b.step_spring(1.f/60.f);
    assert(std::abs(b.overscrollY)<.2f);
    // Same elapsed time, different frame rates: no sticky elastic displacement
    // when a WebGL screenshot or slow host produces occasional long frames.
    Buffer manyFrames,oneFrame;
    manyFrames.push_edge(-3000.f);oneFrame.push_edge(-3000.f);
    for(int i=0;i<30;++i)manyFrames.step_spring(1.f/120.f);
    oneFrame.step_spring(.25f);
    assert(std::abs(manyFrames.overscrollY-oneFrame.overscrollY)<.01f);
    assert(std::abs(manyFrames.springVelocityY-oneFrame.springVelocityY)<.01f);
    b.push_edge(120.f);assert(b.overscrollY>0.f);
    b.reduceMotion=true;b.step_spring(1.f/60.f);
    assert(b.overscrollY==0.f&&b.springVelocityY==0.f);
    b.push_edge(-60.f);assert(b.overscrollY==0.f);
}
