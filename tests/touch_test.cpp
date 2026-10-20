#include "touch.hpp"
#include <cassert>
#include <cmath>
#include <vector>
using touch::Contact;using touch::Type;
int main() {
    touch::Controller g;
    auto a=g.update({Contact{11,20,30}},1.);
    assert(a.size()==2&&a.back().type==Type::Down);
    a=g.update({Contact{11,20,33}},1.02);
    assert(a.size()==1&&a[0].type==Type::Move);
    a=g.update({},1.03);
    assert(a.size()==1&&a[0].type==Type::Up);
    a=g.update({{1,20,30}},2.);
    a=g.update({{1,20,30},{2,80,30}},2.01);
    assert(a.size()==1&&a[0].type==Type::Cancel);
    a=g.update({{1,25,55},{2,85,55}},2.03);
    assert(a.size()==1&&a[0].type==Type::Scroll);
    a=g.update({{1,20,45},{2,110,45}},2.04);
    bool zoom=false;
    for(auto&e:a) if(e.type==Type::Zoom)zoom=true;
    assert(zoom);
    a=g.update({{1,20,45}},2.05);
    assert(a.empty()); // Second finger ending may not click.
    a=g.update({},2.06);
    assert(g.tick(2.075).size()==1); // Multi-finger kinetic scroll survives final lift.
    g.reset();
    a=g.update({{1,10,10}},3.);
    a=g.tick(3.60);
    bool context=false;for(auto&e:a)if(e.type==Type::LongPress)context=true;
    assert(context);
    assert(g.update({},3.62).empty());
    g.reset();
    a=g.update({{1,10,10}},4.);
    a=g.update({{1,10,45}},4.02,true);
    assert(a.size()==2&&a[0].type==Type::Cancel&&a.back().type==Type::Scroll);
    a=g.update({},4.04);
    assert(g.tick(4.055).size()==1); // inertial scroll
    // Tick cadence must not mutate the input velocity sampling clock.
    g.reset();
    g.update({{9,0,0}},10.);
    g.tick(10.016);
    a=g.update({{9,0,40}},10.020);
    assert(a.size()==2 && a.back().type==Type::Scroll);
    assert(std::abs(a.back().second+40.f)<.001f); // Exact drag pixels, no conversion
    g.update({{9,0,60}},10.040);
    g.update({},10.041);
    const auto inertia1=g.tick(10.056);
    assert(!inertia1.empty());
    g.reset();
    // Tap must produce a normal button release; scroll swipe must cancel it.
    g.reset();
    a=g.update({{1,30,30}},4.4);
    assert(a.back().type==Type::Down);
    a=g.update({},4.43);
    assert(a.size()==1&&a.back().type==Type::Up);
    g.reset();
    a=g.update({{1,30,30}},4.6);
    a=g.update({{1,31,75}},4.62,true);
    assert(a.size()==2&&a.front().type==Type::Cancel);
    assert(a.back().type==Type::Scroll);
    a=g.update({},4.64);
    for(const auto& evt:a)assert(evt.type!=Type::Up);
    g.reset();
    a=g.update({{1,10,10}},5.);
    a=g.update({{1,10,50}},5.02,false);
    assert(a.size()==1&&a[0].type==Type::Move); // Active slider: drag, no scroll.
    // Interrupted tap: never turn an OS cancellation into a click.
    g.reset();
    a=g.update({{3,80,80}},20.);
    assert(a.back().type==Type::Down);
    a=g.cancel();
    assert(a.size()==1 && a[0].type==Type::Cancel);
    assert(g.tick(20.016).empty());
    assert(g.update({},20.020).empty());
    // Cancel while coasting stops momentum immediately.
    g.update({{4,10,10}},21.);
    a=g.update({{4,10,90}},21.020,true);
    assert(a.back().type==Type::Scroll && a.back().second==-80.f);
    g.update({},21.024);
    g.cancel();
    assert(g.tick(21.040).empty());
    // Native long press must not release as a primary-button tap.
    g.update({{5,20,20}},22.);
    a=g.tick(22.56);
    assert(a.size()==2 && a[0].type==Type::Cancel &&
           a[1].type==Type::LongPress);
    assert(g.update({},22.6).empty());
    return 0;
}
