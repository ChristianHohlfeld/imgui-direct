#!/usr/bin/env python3
"""Build-only conversion of OFL licensed Inter to static C++ bytes.

No runtime files/font lookup, no additional library. The font is embedded
in the final executable and WebAssembly module. Regenerate only when its
pinned source changes; source and license are tracked by CMake.
"""
import pathlib
import sys

source = pathlib.Path(sys.argv[1]).read_bytes()
output = pathlib.Path(sys.argv[2])
if len(source) < 100000 or source[:4] not in (b"\x00\x01\x00\x00", b"OTTO", b"ttcf"):
    raise SystemExit("Invalid or unexpectedly small OpenType font")
output.parent.mkdir(parents=True, exist_ok=True)
with output.open("w", encoding="ascii") as file:
    file.write('#include "ui/font_data.hpp"\nnamespace ui {\n')
    file.write('alignas(16) const unsigned char inter_font_data[] = {\n')
    for index in range(0, len(source), 32):
        file.write("  " + ",".join(str(byte) for byte in source[index:index+32]) + ",\n")
    file.write('};\nconst int inter_font_size = sizeof(inter_font_data);\n}\n')
