#!/usr/bin/env python3
"""mktemplate.py OUT: writes a Wimp template file with two windows for the dialog tests:
 "main"  - four buttons (Static, Transient, Old, Quit)
 "dlg"   - a dialogue box: label, writable icon (indirected, with a validation string), an option button, three radio buttons (ESG 1), Cancel and OK.
Indirected data lives after the icons; the pointers in the file are offsets from the start of the window definition."""
import struct, sys
def box(a,b,c,d): return struct.pack('<4i', a,b,c,d)
def winhdr(vis, work, title, nicons, flags=0x8F000012):
    w  = box(*vis) + struct.pack('<2i', 0, 0) + struct.pack('<i', -1) + struct.pack('<I', flags)
    w += bytes([7, 2, 7, 1, 3, 1, 12, 0]) + box(*work)
    w += struct.pack('<I', 0x19) + struct.pack('<I', 0) + struct.pack('<I', 1) + struct.pack('<HH', 100, 100)
    assert len(title) <= 11
    w += title.encode().ljust(12, b'\0') + struct.pack('<I', nicons)
    return w
def icon(x0,y0,x1,y1,flags,text): assert len(text) <= 11; return box(x0,y0,x1,y1) + struct.pack('<I', flags) + text.encode().ljust(12, b'\0')
def icon_ind(x0,y0,x1,y1,flags,buf,val,size): return box(x0,y0,x1,y1) + struct.pack('<I', flags) + struct.pack('<3i', buf, val, size)
BTN  = 0x07000019 | (1 << 2) | (1 << 5) | (3 << 12)      # text, border, filled, centred, click button
LAB  = 0x07000011                                        # text, vcentred
def entry(name, data, base):
    return struct.pack('<3i', base, len(data), 1) + name.ljust(12, b'\0')
# window "main"
main  = winhdr((300,300,900,640), (0,-340,600,0), 'DLDialogs', 4)
main += icon(20,-60,580,-20, 0x07000009 | (1 << 4), 'Dialogs')
main += icon(20,-140,280,-90, BTN, 'Static')
main += icon(320,-140,580,-90, BTN, 'Transient')
main += icon(20,-220,280,-170, BTN, 'Quit')
# window "dlg": the indirected data follows the 9 icons (header 88 + 9*32 = 376)
HDR = 88; N = 8; base = HDR + N * 32
name_buf  = base;        name_val = base + 32          # 32 bytes of buffer, then the validation string
shout_buf = base + 64;   shout_val = base + 96
red_buf = base + 112; grn_buf = base + 128; blu_buf = base + 144; radio_val = base + 160
data_end = base + 192
dlg  = winhdr((300,300,780,560), (0,-260,480,0), 'Dialog', N)
dlg += icon(20,-70,110,-20, LAB, 'Name:')
dlg += icon_ind(120,-70,460,-20, 0x0700F135, name_buf, name_val, 32)                       # writable: text, border, filled, vcentre, indirected, type 15
dlg += icon_ind(20,-110,200,-80, 0x0700B113, shout_buf, shout_val, 16)                     # option: text, sprite, vcentre, indirected, type 11 with ESG 0 (the Wimp toggles it; type 12 is reserved)
dlg += icon_ind(20,-150,200,-120, 0x0701B113, red_buf, radio_val, 16)                      # radio ESG 1: type 11
dlg += icon_ind(20,-190,200,-160, 0x0701B113, grn_buf, radio_val, 16)
dlg += icon_ind(20,-230,200,-200, 0x0701B113, blu_buf, radio_val, 16)
dlg += icon(240,-250,350,-190, BTN, 'Cancel')
dlg += icon(360,-250,470,-190, BTN, 'OK')
def pad(s, n): return s.encode().ljust(n, b'\0')
ind  = pad('Fred', 32) + pad('A a-zA-Z0-9 ', 32)
ind += pad('Shout', 32) + pad('Soptoff,opton', 16)
ind += pad('Red', 16) + pad('Green', 16) + pad('Blue', 16) + pad('Sradiooff,radioon', 32)
assert len(dlg) == HDR + N * 32 and len(ind) == data_end - base
dlg_all = dlg + ind
# the Wimp wants the ESG 1 radio icons to read 'selected' for the first: set bit 21 on Red
off = HDR + 3 * 32 + 16
dlg_all = dlg_all[:off] + struct.pack('<I', struct.unpack('<I', dlg_all[off:off+4])[0] | (1 << 21)) + dlg_all[off+4:]
idx = 16 + 2 * 24 + 4
e1 = struct.pack('<3i', idx, len(main), 1) + b'main\r\0\0\0\0\0\0\0'
e2 = struct.pack('<3i', idx + len(main), len(dlg_all), 1) + b'dlg\r\0\0\0\0\0\0\0\0'
out = struct.pack('<4i', -1, 0, 0, 0) + e1 + e2 + struct.pack('<i', 0) + main + dlg_all
open(sys.argv[1], 'wb').write(out)
print('Templates:', len(out), 'bytes')
