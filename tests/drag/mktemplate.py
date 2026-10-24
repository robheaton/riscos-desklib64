#!/usr/bin/env python3
"""mktemplate.py OUT: writes a Wimp template file with one window "main": a draggable sprite icon (click/drag type), a Start button, a Stop button and a Quit button."""
import struct, sys
def box(a,b,c,d): return struct.pack('<4i', a,b,c,d)
def icon(x0,y0,x1,y1,flags,text): assert len(text) <= 11; return box(x0,y0,x1,y1) + struct.pack('<I', flags) + text.encode().ljust(12, b'\0')
BTN = 0x07000019 | (1 << 2) | (1 << 5) | (3 << 12)
w  = box(300,300,900,640) + struct.pack('<2i', 0, 0) + struct.pack('<i', -1) + struct.pack('<I', 0x8F000012)
w += bytes([7, 2, 7, 1, 3, 1, 12, 0]) + box(0, -340, 600, 0)
w += struct.pack('<I', 0x19) + struct.pack('<I', 0) + struct.pack('<I', 1) + struct.pack('<HH', 100, 100)
w += b'DLDrag'.ljust(12, b'\0') + struct.pack('<I', 5)
w += icon(20,-60,580,-20, 0x07000009 | (1 << 4), 'Drag test')
w += icon(40,-200,160,-80, 0x00000002 | (1 << 3) | (1 << 4) | (6 << 12), 'file_fff')        # a sprite icon of the Wimp pool, centred, click/drag
w += icon(220,-140,380,-90, BTN, 'Start')
w += icon(220,-220,380,-170, BTN, 'Stop')
w += icon(420,-140,580,-90, BTN, 'Quit')
idx = 16 + 24 + 4
e1 = struct.pack('<3i', idx, len(w), 1) + b'main\r\0\0\0\0\0\0\0'
out = struct.pack('<4i', -1, 0, 0, 0) + e1 + struct.pack('<i', 0) + w
open(sys.argv[1], 'wb').write(out)
print('Templates:', len(out), 'bytes')
