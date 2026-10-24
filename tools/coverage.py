#!/usr/bin/env python3
"""coverage.py TC: which library functions do the test programs reach?

Compiles each tests/*/*.c to an object (not linked), lists the library functions they call directly, then follows the calls between the library's own objects (nm on libDeskLib64.a) to find the functions that the tests reach.  "Reached" means that the test code can call the function, not that its result was checked.  Run ../build.sh first.  Writes docs/COVERAGE.md."""
import collections, glob, os, re, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__)); R = os.path.dirname(HERE)
TC = sys.argv[1]
CC = TC + '/bin/aarch64-riscos-gcc'; NM = TC + '/bin/aarch64-riscos-nm'
LIB = R + '/dist/lib/libDeskLib64.a'; INC = R + '/dist/include'
tmp = tempfile.mkdtemp()
objs = []
for f in sorted(glob.glob(R + '/tests/*/*.c')):
    o = os.path.join(tmp, os.path.basename(f)[:-2] + '.o')
    subprocess.run([CC, '-munixlib', '-O0', '-fsigned-char', '-I', INC, '-c', f, '-o', o], check=True, stderr=subprocess.DEVNULL)
    objs.append(o)
used = set()
for l in subprocess.run([NM, '-u'] + objs, capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) == 2 and p[0] == 'U': used.add(p[1])
defs, und = {}, collections.defaultdict(set)
for l in subprocess.run([NM, '-A', '-g', LIB], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) != 3: continue
    member = p[0].split(':')[1]
    if p[1] == 'U': und[member].add(p[2])
    elif p[1] == 'T': defs[p[2]] = member
reach, stack, seen = set(), [s for s in used if s in defs], set()
while stack:
    s = stack.pop()
    if s in reach: continue
    reach.add(s)
    m = defs[s]
    if m in seen: continue
    seen.add(m)
    stack.extend(u for u in und[m] if u in defs and u not in reach)
mods = collections.OrderedDict()
for s in sorted(defs):
    if s.startswith('_') and not s.startswith('__') and False: continue
    mods.setdefault(s.split('_')[0] if '_' in s else '(no prefix)', []).append(s)
out = ['# DeskLib64 - test coverage', '',
       'Made by `tools/coverage.py`. A function is **called** if a test program calls it, and **reached** if a test calls it or a function that it calls does (so `Wimp_Poll`, which `Event_Poll` calls, counts). Reached means the test code can run it, not that its result was checked: `docs/STATUS.md` says what each test checks.', '',
       f'**{len(reach)} of {len(defs)} library functions are reached ({sum(s in used for s in defs)} are called directly).**', '',
       '| Module | Called | Reached | Total | Not reached |', '|---|---:|---:|---:|---|']
for p, ss in mods.items():
    c = sum(s in used for s in ss); r = sum(s in reach for s in ss)
    miss = ', '.join(s.split('_', 1)[-1] if '_' in s else s for s in ss if s not in reach)
    out.append(f'| {p} | {c} | {r} | {len(ss)} | {miss} |')
out.append('')
open(R + '/docs/COVERAGE.md', 'w').write('\n'.join(out))
print(out[4])
