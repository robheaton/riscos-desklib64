#!/usr/bin/env python3
"""fixptr.py FILE...: rewrites uses of the 32 bit pointer fields of DeskLib64's OS structures (dl_ptr) in C source: a read becomes ((char *) DL_P (x)) (DL_P (x) for sprite areas
and menus), an assignment x = v; becomes x = DL_PTR (v);.  Only the field chains given in FIELDS are touched."""
import re, sys
EXPR = r'[A-Za-z_]\w*(?:(?:\.|->)[A-Za-z_]\w*|\[[^\]]*\])*'
FIELDS = [r'indirecttext\.buffer', r'indirecttext\.validstring', r'indirectsprite\.name', r'indirectsprite\.spritearea', r'submenu\.menu', r'ramfetch\.buffer', r'ramtransmit\.buffer', r'shapedata']
PLAIN = ('spritearea', 'submenu.menu')            # not char *: use DL_P (x) alone
pat = re.compile(r'(?<![\w.>])(' + EXPR + r'\.(?:' + '|'.join(FIELDS) + r'))(?!\w)')
def fix(text):
    out = []
    for line in text.split('\n'):
        # assignment: LHS = RHS;
        m = re.match(r'^(\s*)(' + EXPR + r'\.(?:' + '|'.join(FIELDS) + r'))\s*=\s*(.*);(\s*(?:/\*.*\*/)?)$', line)
        if m and not re.match(r'^\s*(DL_PTR|\(dl_ptr\))', m.group(3)):
            out.append('%s%s = DL_PTR (%s);%s' % (m.group(1), m.group(2), m.group(3), m.group(4))); continue
        def rd(mm):
            e = mm.group(1)
            return 'DL_P (%s)' % e if e.endswith(PLAIN) else '((char *) DL_P (%s))' % e
        out.append(pat.sub(rd, line))
    return '\n'.join(out)
for f in sys.argv[1:]:
    s = open(f, encoding='latin-1').read(); n = fix(s)
    if n != s: open(f, 'w', encoding='latin-1').write(n); print('fixed', f)
