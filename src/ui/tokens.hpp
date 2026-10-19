#pragma once
// Daisy-like semantic theme tokens, no external dependencies (even no Dear ImGui).
#include <cstdint>
namespace ui {
struct Color { float r, g, b, a=1.f; };
constexpr Color hex(std::uint32_t value) {
    return { float((value>>16)&255)/255.f, float((value>>8)&255)/255.f, float(value&255)/255.f, 1.f };
}
struct Palette {
    Color base100, base200, base300, baseContent;
    Color primary, primaryContent;
    Color secondary, secondaryContent;
    Color neutral, neutralContent;
    Color info, success, warning, error;
};
inline constexpr Palette slateDark {
    hex(0x111827),hex(0x1b2638),hex(0x334155),hex(0xf1f5f9),
    hex(0x60a5fa),hex(0x0b1220),
    hex(0x334155),hex(0xf8fafc),
    hex(0x475569),hex(0xffffff),
    hex(0x38bdf8),hex(0x4ade80),hex(0xfbbf24),hex(0xf87171)
};
inline constexpr Palette slateLight {
    hex(0xf8fafc),hex(0xffffff),hex(0xe2e8f0),hex(0x0f172a),
    hex(0x2563eb),hex(0xffffff),
    hex(0xe2e8f0),hex(0x0f172a),
    hex(0x334155),hex(0xffffff),
    hex(0x0284c7),hex(0x15803d),hex(0xa16207),hex(0xb91c1c)
};
constexpr Color blend(Color a,Color b,float bWeight) {
    return {a.r*(1-bWeight)+b.r*bWeight,a.g*(1-bWeight)+b.g*bWeight,
            a.b*(1-bWeight)+b.b*bWeight,a.a*(1-bWeight)+b.a*bWeight};
}
}
