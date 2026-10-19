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
}
