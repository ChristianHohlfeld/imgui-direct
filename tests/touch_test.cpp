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
    assert(a.size()==1&&a[0].type==Type::Up);
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
    assert(a.size()==2&&a.back().type==Type::Scroll);
    a=g.update({},4.04);
    assert(g.tick(4.055).size()==1); // inertial scroll
    // Tick cadence must not mutate the input velocity sampling clock.
    g.reset();
    g.update({{9,0,0}},10.);
    g.tick(10.016);
    a=g.update({{9,0,40}},10.020);
    assert(a.size()==2 && a.back().type==Type::Scroll);
    assert(std::abs(a.back().second+40.f/60.f)<.001f); // No touch-slop loss
    g.update({{9,0,60}},10.040);
    g.update({},10.041);
    const auto inertia1=g.tick(10.056);
    assert(!inertia1.empty());
    g.reset();
    a=g.update({{1,10,10}},5.);
    a=g.update({{1,10,50}},5.02,false);
    assert(a.size()==1&&a[0].type==Type::Move); // Active slider: drag, no scroll.
    return 0;
}
