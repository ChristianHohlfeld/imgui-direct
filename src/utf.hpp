#pragma once
#include <cstdint>
#include <string>
#include <string_view>
// JNI's GetStringUTFChars returns modified UTF-8; use proper UTF-16 conversion
// instead so emoji and surrogate pairs survive unchanged.
namespace unicode {
inline void append8(std::string& out,std::uint32_t cp) {
    if(cp<=0x7f) out+=(char)cp;
    else if(cp<=0x7ff){out+=(char)(0xc0|(cp>>6));out+=(char)(0x80|(cp&63));}
    else if(cp<=0xffff){out+=(char)(0xe0|(cp>>12));out+=(char)(0x80|((cp>>6)&63));out+=(char)(0x80|(cp&63));}
    else {out+=(char)(0xf0|(cp>>18));out+=(char)(0x80|((cp>>12)&63));out+=(char)(0x80|((cp>>6)&63));out+=(char)(0x80|(cp&63));}
}
inline std::string utf16_to_utf8(std::u16string_view text) {
    std::string out;out.reserve(text.size()*2);
    for(size_t i=0;i<text.size();++i) {
        std::uint32_t cp=text[i];
        if(cp>=0xd800&&cp<=0xdbff) {
            if(i+1<text.size()&&text[i+1]>=0xdc00&&text[i+1]<=0xdfff)
                cp=0x10000+((cp-0xd800)<<10)+(text[++i]-0xdc00);
            else cp=0xfffd;
        } else if(cp>=0xdc00&&cp<=0xdfff) cp=0xfffd;
        append8(out,cp);
    }
    return out;
}
inline std::u16string utf8_to_utf16(std::string_view text) {
    std::u16string out;
    for(size_t i=0;i<text.size();) {
        const auto ch=(unsigned char)text[i++];
        std::uint32_t cp=0xfffd;
        unsigned extra=0;
        if(ch<0x80)cp=ch;
        else if(ch>=0xc2&&ch<=0xdf){cp=ch&31;extra=1;}
        else if(ch>=0xe0&&ch<=0xef){cp=ch&15;extra=2;}
        else if(ch>=0xf0&&ch<=0xf4){cp=ch&7;extra=3;}
        if(extra) {
            const size_t begin=i;
            bool valid=i+extra<=text.size();
            for(unsigned j=0;valid&&j<extra;++j) {
                unsigned b=(unsigned char)text[i++];
                if((b&0xc0)!=0x80)valid=false;
                else cp=(cp<<6)|(b&63);
            }
            if(!valid||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)||
               (extra==1&&cp<0x80)||(extra==2&&cp<0x800)||(extra==3&&cp<0x10000)) {
                cp=0xfffd;i=begin;
            }
        }
        if(cp<=0xffff)out+=(char16_t)cp;
        else {cp-=0x10000;out+=(char16_t)(0xd800+(cp>>10));out+=(char16_t)(0xdc00+(cp&1023));}
    }
    return out;
}
}
