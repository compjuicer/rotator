"""Convert a GEM/GDOS .FNT bitmap font to BDF (then use bdf2ttf.py).

usage: fnt2bdf.py in.fnt out.bdf
"""
import struct
import sys

src, dst = sys.argv[1:3]
d = open(src, "rb").read()
e = ">" if (struct.unpack_from(">H", d, 66)[0] & 4) else "<"
(first, last) = struct.unpack_from(e + "HH", d, 36)
top, ascent, half, descent, bottom = struct.unpack_from(e + "5h", d, 40)
off_tab, dat_tab = struct.unpack_from(e + "II", d, 72)
form_w, form_h = struct.unpack_from(e + "HH", d, 80)
offs = struct.unpack_from(e + f"{last - first + 2}H", d, off_tab)
rows = [d[dat_tab + r * form_w: dat_tab + (r + 1) * form_w] for r in range(form_h)]


def bit(row, x):
    return rows[row][x >> 3] >> (7 - (x & 7)) & 1


cell = max(offs[i + 1] - offs[i] for i in range(last - first + 1))
base = top
out = ["STARTFONT 2.1", f"FONT -gem-{src}", f"SIZE {form_h} 75 75", f"FONTBOUNDINGBOX {cell} {form_h} 0 {base - form_h}",
       "STARTPROPERTIES 2", f"FONT_ASCENT {base}", f"FONT_DESCENT {form_h - base}", "ENDPROPERTIES",
       f"CHARS {last - first + 1}"]
for i in range(last - first + 1):
    c = first + i
    x0, w = offs[i], offs[i + 1] - offs[i]
    out += [f"STARTCHAR c{c}", f"ENCODING {c}", f"SWIDTH {w * 72} 0", f"DWIDTH {w} 0",
            f"BBX {max(w, 1)} {form_h} 0 {base - form_h}", "BITMAP"]
    nb = (max(w, 1) + 7) // 8
    for r in range(form_h):
        v = 0
        for x in range(w):
            v |= bit(r, x0 + x) << (nb * 8 - 1 - x)
        out.append(f"{v:0{nb * 2}X}")
    out.append("ENDCHAR")
out.append("ENDFONT")
open(dst, "w").write("\n".join(out) + "\n")
print(f"{last - first + 1} glyphs, cell {cell}x{form_h}, ascent {base} ({'BE' if e == '>' else 'LE'}) -> {dst}")
