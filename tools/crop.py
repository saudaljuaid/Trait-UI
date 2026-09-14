#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Cut a rectangle out of a rendered frame and magnify it, for looking at.

    python3 tools/crop.py build/desktop.png out.png X Y W H [SCALE]
"""
import struct, sys, zlib, binascii
from pathlib import Path


def read(path):
    d = Path(path).read_bytes(); p = 8; comp = bytearray()
    w = h = bd = ct = None
    while p < len(d):
        L = struct.unpack(">I", d[p:p + 4])[0]
        k = d[p + 4:p + 8]; b = d[p + 8:p + 8 + L]
        if k == b"IHDR":
            w, h, bd, ct = struct.unpack(">IIBB", b[:10])
        elif k == b"IDAT":
            comp += b
        elif k == b"IEND":
            break
        p += L + 12
    raw = zlib.decompress(bytes(comp))
    ch = {0: 1, 2: 3, 4: 2, 6: 4}[ct]
    bpp = ch * bd // 8
    rb = w * ch * bd // 8
    rows = []; prev = bytearray(rb); i = 0
    for y in range(h):
        f = raw[i]; i += 1
        line = bytearray(raw[i:i + rb]); i += rb
        for x in range(rb):
            a = line[x - bpp] if x >= bpp else 0
            b2 = prev[x]; c = prev[x - bpp] if x >= bpp else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + b2) & 255
            elif f == 3:
                line[x] = (line[x] + (a + b2) // 2) & 255
            elif f == 4:
                pa = abs(b2 - c); pb = abs(a - c); pc = abs(a + b2 - 2 * c)
                pr = a if (pa <= pb and pa <= pc) else (b2 if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        rows.append(bytes(line)); prev = line
    return w, h, ch, rows


def chunk(tag, body):
    return (struct.pack(">I", len(body)) + tag + body +
            struct.pack(">I", binascii.crc32(tag + body) & 0xFFFFFFFF))


def main():
    src, dst = sys.argv[1], sys.argv[2]
    x0, y0, cw, chh = (int(v) for v in sys.argv[3:7])
    scale = int(sys.argv[7]) if len(sys.argv) > 7 else 3
    w, h, ch, rows = read(src)
    if x0 < 0:
        x0 += w
    if y0 < 0:
        y0 += h
    cw = min(cw, w - x0); chh = min(chh, h - y0)
    buf = bytearray()
    for y in range(y0, y0 + chh):
        ln = rows[y]
        for _ in range(scale):
            buf.append(0)
            for x in range(x0, x0 + cw):
                o = x * ch
                buf += bytes((ln[o], ln[o + 1], ln[o + 2])) * scale
    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    Path(dst).write_bytes(
        b"\x89PNG\r\n\x1a\n" +
        chunk(b"IHDR", struct.pack(">IIBBBBB", cw * scale, chh * scale,
                                   8, 2, 0, 0, 0)) +
        chunk(b"IDAT", zlib.compress(bytes(buf), 6)) + chunk(b"IEND", b""))
    print("wrote %s (%dx%d)" % (dst, cw * scale, chh * scale))


if __name__ == "__main__":
    main()
