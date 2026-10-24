#!/usr/bin/env python3
"""mktemplate.py OUT: writes a Wimp template file with one window "main" (two plain text icons: a hint and a Quit button), without indirected data."""
import struct, sys
def box(a,b,c,d): return struct.pack('<4i', a,b,c,d)
win  = box(300, 300, 900, 640)                           # visible area
win += struct.pack('<2i', 0, 0)                          # scroll
win += struct.pack('<i', -1)                             # behind
win += struct.pack('<I', 0x8F000012)                     # flags
win += bytes([7, 2, 7, 1, 3, 1, 12, 0])                  # colours
win += box(0, -340, 600, 0)                              # work area
win += struct.pack('<I', 0x19)                           # title icon flags
win += struct.pack('<I', 0)                              # work area flags
win += struct.pack('<I', 1)                              # sprite area: the Wimp pool
win += struct.pack('<HH', 100, 100)                      # minimum size
win += b'DLDemo2\0\0\0\0\0'                              # title (12 bytes)
win += struct.pack('<I', 2)                              # icons
def icon(x0, y0, x1, y1, flags, text):
    assert len(text) <= 11, 'the text of an icon that is not indirected is at most 11 characters (a 12 byte field with its terminator)'
    return box(x0, y0, x1, y1) + struct.pack('<I', flags) + text.encode().ljust(12, b'\0')
data = win
data += icon(20, -60, 580, -20, 0x07000009 | (1 << 4), 'Right-click')          # text, hcentred, vcentred; fg 7
data += icon(200, -320, 400, -250, 0x07000019 | (1 << 2) | (1 << 5) | (3 << 12), 'Quit')       # text, border, filled, centred, button type click
index_end = 16 + 24 + 4
entry = struct.pack('<3i', index_end, len(data), 1) + b'main\r\0\0\0\0\0\0\0'
out = struct.pack('<4i', -1, 0, 0, 0) + entry + struct.pack('<i', 0) + data
open(sys.argv[1], 'wb').write(out)
print('Templates:', len(out), 'bytes')
