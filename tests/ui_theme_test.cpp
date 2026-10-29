#include "ui/tokens.hpp"
#include <cassert>
#include <cmath>
using namespace ui;
constexpr bool rgb_valid(Color c) {
    return c.r>=0 && c.r<=1 && c.g>=0 && c.g<=1 && c.b>=0 && c.b<=1;
}
constexpr bool valid(const Palette& p) {
    return rgb_valid(p.base100)&&rgb_valid(p.base200)&&rgb_valid(p.base300)&&
        rgb_valid(p.baseContent)&&rgb_valid(p.primary)&&rgb_valid(p.primaryContent)&&
        rgb_valid(p.secondary)&&rgb_valid(p.secondaryContent)&&rgb_valid(p.neutral)&&
        rgb_valid(p.neutralContent)&&rgb_valid(p.info)&&rgb_valid(p.success)&&
        rgb_valid(p.warning)&&rgb_valid(p.error);
}
static_assert(valid(slateDark) && valid(slateLight));
static_assert(daisyDark.base100.r<daisyLight.base100.r);
static_assert(daisyLight.metrics.radiusSelector==16.f);
static_assert(daisyLight.metrics.radiusField==8.f);
static_assert(daisyLight.metrics.radiusBox==16.f);
static_assert(daisyLight.metrics.border==1.f);
static_assert(daisyLight.metrics.depth && !daisyLight.metrics.noise);
static_assert(slateDark.base100.r==daisyDark.base100.r);
static_assert(slateLight.base100.r==daisyLight.base100.r);
static_assert(slateDark.baseContent.r>slateLight.baseContent.r);
static_assert(slateDark.primary.r!=slateLight.primary.r);
static_assert(hex(0xff0000).r==1.f);
static_assert(blend(hex(0x000000),hex(0xffffff),.5f).r==.5f);
int main(){assert(valid(slateDark)&&valid(slateLight));}
