#!/usr/bin/env python3
"""mktemplate.py OUT: writes a Wimp template file with one window "main": a title icon only (the sprites are drawn by the application's redraw handler)."""
import struct, sys
def box(a,b,c,d): return struct.pack('<4i', a,b,c,d)
def icon(x0,y0,x1,y1,flags,text): assert len(text) <= 11; return box(x0,y0,x1,y1) + struct.pack('<I', flags) + text.encode().ljust(12, b'\0')
w  = box(300,300,1000,720) + struct.pack('<2i', 0, 0) + struct.pack('<i', -1) + struct.pack("<I", 0x8F000002)
w += bytes([7, 2, 7, 1, 3, 1, 12, 0]) + box(0, -420, 700, 0)
w += struct.pack('<I', 0x19) + struct.pack('<I', 0) + struct.pack('<I', 1) + struct.pack('<HH', 100, 100)
w += b'DLSprite'.ljust(12, b'\0') + struct.pack('<I', 1)
w += icon(20,-50,680,-10, 0x07000009 | (1 << 4), 'Sprites')
idx = 16 + 24 + 4
e1 = struct.pack('<3i', idx, len(w), 1) + b'main\r\0\0\0\0\0\0\0'
out = struct.pack('<4i', -1, 0, 0, 0) + e1 + struct.pack('<i', 0) + w
open(sys.argv[1], 'wb').write(out)
print('Templates:', len(out), 'bytes')
