#!/usr/bin/env python3
"""Render harness draw-calls into PNG previews of the real firmware UI.

Usage: python3 render.py [ops.txt] [out_dir]
"""
import sys
from PIL import Image, ImageDraw, ImageFont

S = 3  # upscale
W, H = 320, 170
CHAR_W = {1: 6, 2: 9, 4: 15}
CHAR_H = {1: 8, 2: 16, 4: 26}
BASE = ImageFont.load_default()


def c565(v):
    r = ((v >> 11) & 31) * 255 // 31
    g = ((v >> 5) & 63) * 255 // 63
    b = (v & 31) * 255 // 31
    return (r, g, b)


def draw_text(img, x, y, font, datum, align, fg, bg, text):
    cw, ch = CHAR_W.get(font, 6), CHAR_H.get(font, 8)
    tmp = Image.new("RGB", (max(1, len(text)) * cw * 2 + 4, ch * 2 + 4), c565(bg))
    d = ImageDraw.Draw(tmp)
    d.text((2, 2), text, fill=c565(fg), font=BASE)
    drect = d.textbbox((2, 2), text, font=BASE)
    if drect:
        tmp = tmp.crop((drect[0], drect[1], drect[2], drect[3] + 1))
        tmp = tmp.resize((len(text) * cw, ch), Image.LANCZOS)
    else:
        tmp = tmp.resize((len(text) * cw, ch))
    wpx, hpx = tmp.size
    if align == "C":
        x -= wpx // 2
    elif align == "R":
        x -= wpx
    if datum == 4:  # MC_DATUM: vertical center too
        y -= hpx // 2
    img.paste(tmp, (x, y))


ops_path = sys.argv[1] if len(sys.argv) > 1 else "ops.txt"
out_dir = sys.argv[2] if len(sys.argv) > 2 else "."
names = ["preview_menu_page1.png", "preview_menu_page2.png", "preview_startup.png"]

frames = [[]]
for line in open(ops_path):
    parts = line.rstrip("\n").split(" ", 8)
    kind = parts[0]
    if kind == "FRAME":
        frames.append([])
        continue
    a = list(map(int, parts[1:7]))
    color = int(parts[7], 16)
    text = parts[8] if len(parts) > 8 else ""
    frames[-1].append((kind, a, color, text))

for fi, ops in enumerate(frames):
    img = Image.new("RGB", (W * S, H * S), (255, 255, 255))
    d = ImageDraw.Draw(img)

    def r(xy):
        return [v * S for v in xy]

    for kind, a, color, text in ops:
        col = c565(color)
        x, y, c, dd, e, f = a
        if kind == "fillScreen":
            d.rectangle(r([0, 0, W - 1, H - 1]), fill=col)
        elif kind == "fillRect":
            d.rectangle(r([x, y, x + c - 1, y + dd - 1]), fill=col)
        elif kind == "drawRect":
            d.rectangle(r([x, y, x + c - 1, y + dd - 1]), outline=col)
        elif kind == "fillRoundRect":
            d.rounded_rectangle(r([x, y, x + c - 1, y + dd - 1]), radius=e * S, fill=col)
        elif kind == "drawRoundRect":
            d.rounded_rectangle(r([x, y, x + c - 1, y + dd - 1]), radius=e * S, outline=col)
        elif kind == "drawFastHLine":
            d.line(r([x, y, x + c - 1, y]), fill=col, width=S)
        elif kind == "drawFastVLine":
            d.line(r([x, y, x, y + c - 1]), fill=col, width=S)
        elif kind == "fillCircle":
            d.ellipse(r([x - c, y - c, x + c, y + c]), fill=col)
        elif kind == "drawCircle":
            d.ellipse(r([x - c, y - c, x + c, y + c]), outline=col)
        elif kind == "fillEllipse":
            d.ellipse(r([x - c, y - dd, x + c, y + dd]), fill=col)
        elif kind == "fillTriangle":
            d.polygon(r([x, y, c, dd, e, f]), fill=col)
        elif kind == "drawLine":
            d.line(r([x, y, c, dd]), fill=col)
        elif kind == "drawPixel":
            d.rectangle([x * S, y * S, x * S + S - 1, y * S + S - 1], fill=col)
        elif kind == "text":
            bghex, _, t = text.partition("|")
            draw_text(img, x * S, y * S, c, dd, chr(e), color, int(bghex, 16), t)

    out = f"{out_dir}/{names[fi]}"
    img.save(out)
    print("wrote", out)
