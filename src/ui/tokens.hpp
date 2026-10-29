#pragma once
// daisyUI v5 built-in light/dark themes, represented as native sRGB tokens.
// CSS original: saadeghi/daisyui packages/daisyui/src/themes/{light,dark}.css
// Source colors use OKLCH; each literal below is converted and rounded to 8-bit
// sRGB for use with Dear ImGui (no CSS, DOM or runtime conversion/dependency).
#include <cstdint>
namespace ui {
struct Color { float r, g, b, a=1.f; };
constexpr Color hex(std::uint32_t value) {
    return {float((value>>16)&255)/255.f,float((value>>8)&255)/255.f,float(value&255)/255.f,1.f};
}
constexpr Color blend(Color a,Color b,float weight) {
    return {a.r*(1-weight)+b.r*weight,a.g*(1-weight)+b.g*weight,
            a.b*(1-weight)+b.b*weight,a.a*(1-weight)+b.a*weight};
}
// daisyUI's base-size and radius tokens: rem evaluated at 16 logical px.
// Scaling is applied by the rendering layer once; no cumulative changes.
struct Metrics {
    float radiusSelector=8.f; // --radius-selector: .5rem
    float radiusField=4.f;    // --radius-field: .25rem
    float radiusBox=8.f;      // --radius-box: .5rem
    float sizeSelector=4.f;   // --size-selector: .25rem
    float sizeField=4.f;      // --size-field: .25rem
    float border=1.f;        // --border: 1px
    bool depth=true;         // --depth: 1
    bool noise=false;        // --noise: 0
};
struct Palette {
    // Preserve legacy aggregate-initializer order and native API.
    Color base100, base200, base300, baseContent;
    Color primary, primaryContent;
    Color secondary, secondaryContent;
    Color neutral, neutralContent;
    Color info, success, warning, error;
    Color accent=hex(0x00d3bb), accentContent=hex(0x084d49);
    Color infoContent=hex(0x042e49), successContent=hex(0x004c39);
    Color warningContent=hex(0x793205), errorContent=hex(0x4d0218);
    Metrics metrics={};
};
inline constexpr Palette daisyLight {
    hex(0xffffff),hex(0xf8f8f8),hex(0xeeeeee),hex(0x18181b),
    hex(0x422ad5),hex(0xe0e7ff),hex(0xf43098),hex(0xf9e4f0),
    hex(0x09090b),hex(0xe4e4e7),
    hex(0x00bafe),hex(0x00d390),hex(0xfcb700),hex(0xff627d),
    hex(0x00d3bb),hex(0x084d49),
    hex(0x042e49),hex(0x004c39),hex(0x793205),hex(0x4d0218),Metrics{}
};
inline constexpr Palette daisyDark {
    hex(0x1d232a),hex(0x191e24),hex(0x15191e),hex(0xecf9ff),
    hex(0x605dff),hex(0xedf1fe),hex(0xf43098),hex(0xf9e4f0),
    hex(0x09090b),hex(0xe4e4e7),
    hex(0x00bafe),hex(0x00d390),hex(0xfcb700),hex(0xff627d),
    hex(0x00d3bb),hex(0x084d49),
    hex(0x042e49),hex(0x004c39),hex(0x793205),hex(0x4d0218),Metrics{}
};
// Legacy aliases retained for existing embedders. The old Slate colors
// are intentionally replaced by daisyUI's actual base theme data.
inline constexpr Palette slateDark=daisyDark;
inline constexpr Palette slateLight=daisyLight;
}
