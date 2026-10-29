#pragma once
// Native C++ theme tokens matching the daisyUI v5 base schema.
// No DOM, CSS or extra rendering dependency: values are normalized sRGB.
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
// Corresponds to daisyUI's radius/size/border/depth/noise properties.
// Logical px; style scale is applied once in the ImGui backend.
struct Metrics {
    float radiusSelector=16.f; // --radius-selector
    float radiusField=8.f;     // --radius-field
    float radiusBox=16.f;      // --radius-box
    float sizeSelector=4.f;    // --size-selector
    float sizeField=4.f;       // --size-field
    float border=1.f;          // --border
    bool depth=true;          // --depth
    bool noise=false;         // --noise
};
struct Palette {
    // Keep the original 14 fields in place: existing aggregate initializers work.
    Color base100, base200, base300, baseContent;
    Color primary, primaryContent;
    Color secondary, secondaryContent;
    Color neutral, neutralContent;
    Color info, success, warning, error;
    Color accent=hex(0x00bfa6), accentContent=hex(0x002c26);
    Color infoContent=hex(0x002c3c), successContent=hex(0x003324);
    Color warningContent=hex(0x392600), errorContent=hex(0x3e0916);
    Metrics metrics={};
};
// sRGB approximations of daisyUI's OKLCH light/dark base themes.
// Not CSS pixel-equivalence: fonts, shadows and color interpolation can differ.
inline constexpr Palette daisyLight {
    hex(0xffffff),hex(0xf5f5f5),hex(0xe5e5e5),hex(0x1f2937),
    hex(0x4b3df1),hex(0xffffff),hex(0xeb4bba),hex(0xffffff),
    hex(0x272b31),hex(0xffffff),
    hex(0x008ccb),hex(0x008a65),hex(0xe5a100),hex(0xdf4662),
    hex(0x00bfa6),hex(0x003c35),
    hex(0xffffff),hex(0xffffff),hex(0x352800),hex(0xffffff),Metrics{}
};
inline constexpr Palette daisyDark {
    hex(0x1d232a),hex(0x191e24),hex(0x15191e),hex(0xf3f4f6),
    hex(0x7480ff),hex(0x070a1e),hex(0xff75c3),hex(0x2e0821),
    hex(0x292e36),hex(0xffffff),
    hex(0x00bafe),hex(0x00d390),hex(0xfcb700),hex(0xff637d),
    hex(0x00d3bb),hex(0x002b27),
    hex(0x002b3a),hex(0x003622),hex(0x3b2a00),hex(0x3f0713),Metrics{}
};
// Source-compatible historic names, intentionally mapped to the base scheme.
inline constexpr Palette slateDark=daisyDark;
inline constexpr Palette slateLight=daisyLight;
}
