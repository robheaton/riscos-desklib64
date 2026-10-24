#!/usr/bin/env python3
"""mktemplate.py OUT: writes a Wimp template file with two windows for the save/drag test:
 "main" - a Save button, a Quit button and a status line is added in code
 "save" - a Save box: a draggable file icon (indirected sprite), a writable filename (indirected), Cancel and OK."""
import struct, sys
def box(a,b,c,d): return struct.pack('<4i', a,b,c,d)
def winhdr(vis, work, title, nicons, flags=0x8F000012):
    assert len(title) <= 11
    w  = box(*vis) + struct.pack('<2i', 0, 0) + struct.pack('<i', -1) + struct.pack('<I', flags)
    w += bytes([7, 2, 7, 1, 3, 1, 12, 0]) + box(*work)
    w += struct.pack('<I', 0x19) + struct.pack('<I', 0) + struct.pack('<I', 1) + struct.pack('<HH', 100, 100)
    w += title.encode().ljust(12, b'\0') + struct.pack('<I', nicons)
    return w
def icon(x0,y0,x1,y1,flags,text): assert len(text) <= 11; return box(x0,y0,x1,y1) + struct.pack('<I', flags) + text.encode().ljust(12, b'\0')
def icon_ind(x0,y0,x1,y1,flags,a,b,c): return box(x0,y0,x1,y1) + struct.pack('<I', flags) + struct.pack('<3i', a, b, c)
BTN  = 0x07000019 | (1 << 2) | (1 << 5) | (3 << 12)
main  = winhdr((300,300,900,640), (0,-340,600,0), 'DLSave', 3)
main += icon(20,-60,580,-20, 0x07000009 | (1 << 4), 'Save test')
main += icon(20,-140,280,-90, BTN, 'Save...')
main += icon(320,-140,580,-90, BTN, 'Quit')
HDR = 88; N = 4; base = HDR + N * 32
spr_buf = base;      name_buf = base + 16; name_val = base + 16 + 128
def pad(s, n): return s.encode().ljust(n, b'\0')
ind = pad("file_fff", 16) + pad('<Wimp$ScrapDir>.dlsave_out', 128) + pad('A', 16)
sv  = winhdr((300,300,780,520), (0,-220,480,0), 'Save', N)
sv += icon_ind(190,-110,290,-20, 0x0000011A | (6 << 12), spr_buf, 1, 12)                         # file icon: sprite, hcentre, vcentre, indirected, click/drag
sv += icon_ind(20,-170,460,-130, 0x0700F135, name_buf, name_val, 128)                            # writable filename
sv += icon(20,-210,230,-180, BTN, 'Cancel')
sv += icon(250,-210,460,-180, BTN, 'Save')
assert len(sv) == base and len(ind) == 16 + 128 + 16
sv_all = sv + ind
idx = 16 + 2 * 24 + 4
e1 = struct.pack('<3i', idx, len(main), 1) + b'main\r\0\0\0\0\0\0\0'
e2 = struct.pack('<3i', idx + len(main), len(sv_all), 1) + b'save\r\0\0\0\0\0\0\0'
out = struct.pack('<4i', -1, 0, 0, 0) + e1 + e2 + struct.pack('<i', 0) + main + sv_all
open(sys.argv[1], 'wb').write(out)
print('Templates:', len(out), 'bytes')
