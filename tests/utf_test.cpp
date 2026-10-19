#include "utf.hpp"
#include <cassert>
#include <string>
int main() {
    const std::string sample="Grüße 🌍 日本語 🧑‍💻";
    auto u=unicode::utf8_to_utf16(sample);
    assert(unicode::utf16_to_utf8(u)==sample);
    assert(unicode::utf16_to_utf8(u"abc")=="abc");
    assert(unicode::utf16_to_utf8(std::u16string(1,(char16_t)0xd800))=="\xef\xbf\xbd");
    assert(unicode::utf8_to_utf16("abc")==u"abc");
    return 0;
}
