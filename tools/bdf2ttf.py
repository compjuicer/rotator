"""Convert a BDF bitmap font to a TrueType font of square pixel outlines.

usage: bdf2ttf.py in.bdf out.ttf "Family Name"
needs fontTools
"""
import sys

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

src, dst, family = sys.argv[1:4]
PX = 64

glyphs, ascent, descent, cell_w = {}, 14, 2, 8
cur = None
with open(src, encoding="latin-1") as f:
    lines = iter(f.read().splitlines())
for line in lines:
    k, _, v = line.partition(" ")
    if k == "FONT_ASCENT":
        ascent = int(v)
    elif k == "FONT_DESCENT":
        descent = int(v)
    elif k == "STARTCHAR":
        cur = {"enc": -1, "dw": 8, "bbx": (8, 16, 0, -2), "rows": []}
    elif k == "ENCODING" and cur is not None:
        cur["enc"] = int(v.split()[0])
    elif k == "DWIDTH" and cur is not None:
        cur["dw"] = int(v.split()[0])
    elif k == "BBX" and cur is not None:
        cur["bbx"] = tuple(int(x) for x in v.split())
    elif k == "BITMAP" and cur is not None:
        for row in lines:
            if row == "ENDCHAR":
                break
            cur["rows"].append(int(row, 16) if row else 0)
        if cur["enc"] >= 0:
            glyphs[cur["enc"]] = cur
        cur = None

order, cmap, outlines, metrics = [".notdef"], {}, {}, {".notdef": (cell_w * PX, 0)}
outlines[".notdef"] = TTGlyphPen(None).glyph()
for enc, g in sorted(glyphs.items()):
    name = f"uni{enc:04X}"
    w, h, xo, yo = g["bbx"]
    nbytes = (w + 7) // 8
    pen = TTGlyphPen(None)
    for r, bits in enumerate(g["rows"]):
        y = yo + h - 1 - r
        x = 0
        while x < w:
            if bits >> (nbytes * 8 - 1 - x) & 1:
                x0 = x
                while x < w and bits >> (nbytes * 8 - 1 - x) & 1:
                    x += 1
                X0, X1, Y0, Y1 = (xo + x0) * PX, (xo + x) * PX, y * PX, (y + 1) * PX
                pen.moveTo((X0, Y0))
                pen.lineTo((X0, Y1))
                pen.lineTo((X1, Y1))
                pen.lineTo((X1, Y0))
                pen.closePath()
            else:
                x += 1
    order.append(name)
    cmap[enc] = name
    outlines[name] = pen.glyph()
    metrics[name] = (g["dw"] * PX, 0)

fb = FontBuilder((ascent + descent) * PX, isTTF=True)
fb.setupGlyphOrder(order)
fb.setupCharacterMap({k: v for k, v in cmap.items() if k >= 32})
fb.setupGlyf(outlines)
fb.setupHorizontalMetrics(metrics)
fb.setupHorizontalHeader(ascent=ascent * PX, descent=-descent * PX)
fb.setupNameTable({"familyName": family, "styleName": "Regular"})
fb.setupOS2(sTypoAscender=ascent * PX, sTypoDescender=-descent * PX, usWinAscent=ascent * PX, usWinDescent=descent * PX)
fb.setupPost()
fb.save(dst)
print(f"{len(order) - 1} glyphs -> {dst}")
