#!/usr/bin/env python3
"""mktemplate.py OUT: writes a Wimp template file with four windows for the pane test:
 "main"  - a resizable window (all the window furniture, a 1000 x 800 work area), used with Pane
 "pane"  - its tool bar: a pane window (no furniture) with two buttons and Quit
 "main2" - another resizable window, used with Pane2
 "pane2" - a pane for it: three buttons, placed by the template (the offset is not given in code)."""
import struct, sys
def box(a,b,c,d): return struct.pack('<4i', a,b,c,d)
def win(vis, work, title, nicons, flags):
    assert len(title) <= 11
    w  = box(*vis) + struct.pack('<2i', 0, 0) + struct.pack('<i', -1) + struct.pack('<I', flags)
    w += bytes([7, 2, 7, 1, 3, 1, 12, 0]) + box(*work)
    w += struct.pack('<I', 0x19) + struct.pack('<I', 0) + struct.pack('<I', 1) + struct.pack('<HH', 100, 100)
    w += title.encode().ljust(12, b'\0') + struct.pack('<I', nicons)
    return w
def icon(x0,y0,x1,y1,flags,text): assert len(text) <= 11; return box(x0,y0,x1,y1) + struct.pack('<I', flags) + text.encode().ljust(12, b'\0')
BTN = 0x07000019 | (1 << 2) | (1 << 5) | (3 << 12)
LAB = 0x07000009 | (1 << 4)
MAINF = 0xFF000012       # new flags valid, back, close, title, toggle size, vscroll, adjust size, hscroll; moveable; auto redraw
PANEF = 0x80000032       # new flags valid; moveable; auto redraw; pane (bit 5)
wins = []
m = win((100,300,700,700), (0,-800,1000,0), 'DLPane', 1, MAINF) + icon(20,-120,980,-80, LAB, 'Pane test')
wins.append((b'main', m))
p = win((120,640,640,700), (0,-60,520,0), 'Tools', 3, PANEF)
p += icon(10,-50,160,-10, BTN, 'One') + icon(180,-50,330,-10, BTN, 'Two') + icon(350,-50,500,-10, BTN, 'Quit')
wins.append((b'pane', p))
m2 = win((800,300,1400,700), (0,-800,1000,0), 'DLPane2', 1, MAINF) + icon(20,-120,980,-80, LAB, 'Pane2 test')
wins.append((b'main2', m2))
p2 = win((820,640,1380,700), (0,-60,560,0), 'Tools2', 3, PANEF)
p2 += icon(10,-50,170,-10, BTN, 'Red') + icon(200,-50,360,-10, BTN, 'Green') + icon(390,-50,550,-10, BTN, 'Blue')
wins.append((b'pane2', p2))
n = len(wins)
idx = 16 + n * 24 + 4
entries = b''; body = b''; off = idx
for name, data in wins:
    entries += struct.pack('<3i', off, len(data), 1) + (name + b'\r').ljust(12, b'\0')
    body += data; off += len(data)
out = struct.pack('<4i', -1, 0, 0, 0) + entries + struct.pack('<i', 0) + body
open(sys.argv[1], 'wb').write(out)
print('Templates:', len(out), 'bytes')
