#!/usr/bin/env python3
"""armv2c.py: translates DeskLib's small ARM assembler SWI veneers (AOF syntax, one or two functions per file) into C for AArch64 RISC OS 64.

Usage: armv2c.py SRCDIR OUTDIR          (SRCDIR = desklib64/src; writes OUTDIR/<Library>/<file>.c and OUTDIR/report.txt)

The registers r0-r15 become 64-bit variables, the flags N Z C V ints; a SWI calls swi_x with r0-r9 (the V flag is the error flag: r0 is then the error block pointer);
loads and stores of words are 32 bit (the OS data structures are); the stack is an array so that the arguments after the fourth (on the stack in the ARM calling convention) are
found where the code looks for them.  The C prototype of each function comes from DeskLib's header (include/*.h).  Branches, data tables and anything not understood make the
file "manual": a stub is written that fails to compile with the reason, and the report lists it.
"""
import os, re, sys, glob

REGS = {'a1': 0, 'a2': 1, 'a3': 2, 'a4': 3, 'v1': 4, 'v2': 5, 'v3': 6, 'v4': 7, 'v5': 8, 'v6': 9, 'sl': 10, 'v7': 10, 'fp': 11, 'v8': 11, 'ip': 12, 'sp': 13, 'lr': 14, 'pc': 15}
for i in range(16): REGS['r%d' % i] = i
COND = {'EQ': 'Z', 'NE': '!Z', 'CS': 'C', 'HS': 'C', 'CC': '!C', 'LO': '!C', 'MI': 'N', 'PL': '!N', 'VS': 'V', 'VC': '!V', 'HI': '(C && !Z)', 'LS': '(!C || Z)',
        'GE': '(N == V)', 'LT': '(N != V)', 'GT': '(!Z && N == V)', 'LE': '(Z || N != V)', 'AL': '1', '': '1'}

class Manual(Exception): pass

def strip_comment(l):
    out, q = '', False
    for ch in l:
        if ch == '"': q = not q
        if ch == ';' and not q: break
        out += ch
    return out.rstrip()

def split_ops(s):
    ops, cur, depth = [], '', 0
    for ch in s:
        if ch in '[{': depth += 1
        if ch in ']}': depth -= 1
        if ch == ',' and depth == 0: ops.append(cur.strip()); cur = ''
        else: cur += ch
    if cur.strip(): ops.append(cur.strip())
    return ops

def load_equs(paths):
    d = {}
    for p in paths:
        try: txt = open(p, encoding='latin-1').read()
        except OSError: continue
        for l in txt.split('\n'):
            m = re.match(r'^(\w+)\s+EQU\s+(.+?)\s*(;.*)?$', l)
            if m: d[m.group(1)] = m.group(2)
    return d

def expr(e, equs):
    e = e.strip()
    e = re.sub(r'&([0-9A-Fa-f]+)', r'0x\1', e)
    e = e.replace(':SHL:', '<<').replace(':SHR:', '>>').replace(':OR:', '|').replace(':AND:', '&').replace(':EOR:', '^').replace(':NOT:', '~')
    def sub(m):
        n = m.group(0)
        if n in equs: return '(%d)' % expr(equs[n], equs)
        raise Manual('unknown symbol ' + n)
    e = re.sub(r'\b[A-Za-z_]\w*\b', lambda m: m.group(0) if re.fullmatch(r'0x[0-9A-Fa-f]+', m.group(0)) else sub(m), e)
    try: return int(eval(e, {}, {}))
    except Exception: raise Manual('expression ' + e)

def parse_regs(s):
    s = s.strip().lstrip('{').rstrip('}')
    out = []
    for part in s.split(','):
        part = part.strip()
        if '-' in part:
            a, b = [x.strip().lower() for x in part.split('-')]
            out += list(range(REGS[a], REGS[b] + 1))
        elif part: out.append(REGS[part.lower()])
    return sorted(set(out))

MN = re.compile(r'^(MOV|MVN|ADD|ADC|SUB|SBC|RSB|AND|ORR|EOR|BIC|CMP|CMN|TST|TEQ|LDR|STR|LDM|STM|SWI|BL|B|MUL|NOP)(EQ|NE|CS|HS|CC|LO|MI|PL|VS|VC|HI|LS|GE|LT|GT|LE|AL)?(S)?(B|BT|T)?(FD|IA|IB|DA|DB|ED|EA|FA)?$')

def parse_mn(m):
    m = m.upper()
    r = MN.match(m)
    if r: return r.groups()
    # old style LDR{cond}B etc. is covered by the pattern above; LDRB{cond}
    r = re.match(r'^(LDR|STR)(B|BT|T)(EQ|NE|CS|HS|CC|LO|MI|PL|VS|VC|HI|LS|GE|LT|GT|LE|AL)$', m)
    if r: return (r.group(1), r.group(3), None, r.group(2), None)
    raise Manual('instruction ' + m)

def op2(o, equs, shifted=None):
    """returns a C expression (uint64_t) for a flexible operand"""
    o = o.strip()
    if o.startswith('#'): return '(uint64_t)(int64_t)(%d)' % expr(o[1:], equs)
    if o.lower() in REGS:
        if REGS[o.lower()] == 13: return '((uint64_t) (uintptr_t) &stk[sp])'
        return 'r[%d]' % REGS[o.lower()]
    raise Manual('operand ' + o)

DATA = {}
REVIEW = []

def data_value(kind, rest, equs):
    rest = rest.strip()
    if kind == 'DCB':
        m = re.match(r'^"([^"]*)"$', rest)
        if not m or len(m.group(1)) > 4: raise Manual('DCB form ' + rest)
        b = m.group(1).encode('latin-1') + b'\0' * (4 - len(m.group(1)))
        return int.from_bytes(b, 'little')
    if kind == 'DCD': return expr(rest, equs) & 0xFFFFFFFF
    raise Manual('data ' + kind)

def translate(path, equs, protos):
    DATA.clear()
    lines = [strip_comment(l) for l in open(path, encoding='latin-1').read().split('\n')]
    funcs, cur = [], None
    exports = set()
    for ln in lines:
        t0 = ln.split()
        if len(t0) == 2 and t0[0].upper() == 'EXPORT': exports.add(t0[1])
    for ln in lines:
        if not ln.strip(): continue
        if ln[0] not in ' \t':
            toks = ln.split()
            label = toks[0]
            rest = ' '.join(toks[1:])
            if label in exports and rest == '':
                cur = {'name': label, 'items': [], 'mode': 'code'}; funcs.append(cur); continue
            if rest and rest.split()[0].upper() in ('DCB', 'DCD'):
                kd, _, val = rest.partition(' ')
                DATA[label] = data_value(kd.upper(), val, equs); continue
            if rest == '' or rest.split()[0].upper() in ('EQU', 'DCD', 'DCB', 'MACRO'):
                if cur is not None and cur['mode'] == 'code' and rest == '':
                    cur['items'].append(('label', label)); continue
            line = rest if rest else ''
            if line == '': continue
            if cur is not None: cur['items'].append(('label', label))
            ln = '\t' + line
        t = ln.split(None, 1)
        mn = t[0].upper(); args = t[1].strip() if len(t) > 1 else ''
        if mn in ('GET', 'PREAMBLE', 'AREA', 'EXPORT', 'IMPORT', 'MACRO', 'MEND', 'ALIGN', 'IDENT', 'ROUT'): continue
        if mn == 'END': break
        if mn == 'STARTCODE':
            cur = {'name': args.split()[0], 'items': [], 'mode': 'code'}; funcs.append(cur); continue
        if cur is None: continue
        if mn in ('DCD', 'DCB') and cur['items'] and cur['items'][-1][0] == 'label':
            lab = cur['items'].pop()[1]
            DATA[lab] = data_value(mn, args, equs); continue
        if mn in ('DCD', 'DCB', 'DCW', '%'):
            raise Manual('data: ' + mn)
        cur['items'].append(('ins', mn, args))
    return funcs

EXTRA = {
    'Wimp_Poll': ('os_error *', ['int', 'void *']), 'Wimp_Poll3': ('os_error *', ['int', 'void *', 'void *']),
    'Wimp_PollIdle': ('os_error *', ['int', 'void *', 'int']), 'Wimp_PollIdle3': ('os_error *', ['int', 'void *', 'int', 'void *']),
    'Font_SetScaleFactor': ('os_error *', ['int', 'int']),
    'Sprite_ReadMaskPixel': ('int', ['void *', 'const char *', 'const int *']), 'Sprite_ReadMaskPixelP': ('int', ['void *', 'const void *', 'const int *']),
    'Sprite_WriteMaskPixel': ('os_error *', ['void *', 'const char *', 'const int *', 'int']), 'Sprite_WriteMaskPixelP': ('os_error *', ['void *', 'const void *', 'const int *', 'int']),
}

def proto_of(name, protos):
    return protos.get(name)

def emit_function(f, equs, protos):
    name = f['name']
    pr = protos.get(name)
    if pr is None: raise Manual('no prototype for ' + name)
    ret, params = pr
    out = []
    # which stack args: params 5.. go to stk
    nparams = len(params)
    body = []
    labels = {it[1] for it in f['items'] if it[0] == 'label'}
    if labels: raise Manual('branch labels in ' + name)
    sp_init = 32
    for it in f['items']:
        _, mn, args = it
        base, cond, s, bsfx, mode = parse_mn(mn)
        cond = cond or ''
        c = COND[cond]
        def emit(st):
            body.append(('if (%s) { %s }' % (c, st)) if c != '1' else st)
        ops = split_ops(args)
        if base in ('B', 'BL'): raise Manual('branch')
        if base == 'NOP': continue
        if base == 'SWI':
            n = expr(args.strip().strip('"'), equs) if not args.strip().startswith('"') else None
            if n is None: raise Manual('swi by name')
            emit('{ uint64_t q[10]; unsigned long f; int i; for (i = 0; i < 10; i++) q[i] = r[i]; f = swi_f (%#x, q); V = f & 1; C = (f >> 1) & 1; Z = (f >> 2) & 1; N = (f >> 3) & 1; for (i = 0; i < 10; i++) r[i] = q[i]; }' % n)
            continue
        if base in ('MOV', 'MVN'):
            rd = REGS[ops[0].lower()]
            if len(ops) > 2: raise Manual('shifted operand')
            v = op2(ops[1], equs)
            if rd == 15:
                if ops[1].lower() in ('lr', 'ip'): body.append(('if (%s) { RETURN }' % c)) if c != '1' else body.append('RETURN')
                else: raise Manual('mov pc')
                continue
            if base == 'MVN': v = '(~%s)' % v
            st = 'r[%d] = %s;' % (rd, v)
            if s: st += ' Z = ((uint32_t) r[%d] == 0); N = (int32_t) r[%d] < 0;' % (rd, rd)
            emit(st); continue
        if base in ('ADD', 'SUB', 'RSB', 'AND', 'ORR', 'EOR', 'BIC', 'ADC', 'SBC'):
            rd = REGS[ops[0].lower()]
            if len(ops) == 2: ops = [ops[0], ops[0], ops[1]]
            if len(ops) > 3: raise Manual('shifted operand')
            a = op2(ops[1], equs); b = op2(ops[2], equs)
            sym = {'ADD': '+', 'SUB': '-', 'AND': '&', 'ORR': '|', 'EOR': '^'}
            if base == 'RSB': e = '(%s - %s)' % (b, a)
            elif base == 'BIC': e = '(%s & ~%s)' % (a, b)
            elif base in sym: e = '(%s %s %s)' % (a, sym[base], b)
            else: raise Manual(base)
            if rd == 13:
                if ops[1].lower() == 'sp' and ops[2].startswith('#') and base in ('ADD', 'SUB'):
                    k = expr(ops[2][1:], equs)
                    if k % 4: raise Manual('sp adjust not a multiple of 4')
                    body.append('sp %s= %d;' % ('+' if base == 'ADD' else '-', k // 4)); continue
                raise Manual('sp arithmetic')
            st = 'r[%d] = %s;' % (rd, e)
            if s: st += ' Z = ((uint32_t) r[%d] == 0); N = (int32_t) r[%d] < 0;' % (rd, rd)
            emit(st); continue
        if base in ('CMP', 'CMN', 'TST', 'TEQ'):
            a = op2(ops[0], equs); b = op2(ops[1], equs)
            if base == 'CMP': emit('{ int64_t d = (int64_t) (int32_t) (%s) - (int64_t) (int32_t) (%s); Z = d == 0; N = d < 0; C = ((uint32_t) (%s) >= (uint32_t) (%s)); V = d > 2147483647LL || d < -2147483648LL; }' % (a, b, a, b))
            elif base == 'TEQ': emit('{ uint32_t d = (uint32_t) (%s) ^ (uint32_t) (%s); Z = d == 0; N = (int32_t) d < 0; }' % (a, b))
            elif base == 'TST': emit('{ uint32_t d = (uint32_t) (%s) & (uint32_t) (%s); Z = d == 0; N = (int32_t) d < 0; }' % (a, b))
            else: raise Manual('CMN')
            continue
        if base in ('LDR', 'STR'):
            rd = REGS[ops[0].lower()]
            if len(ops) == 2 and base == 'LDR' and ops[1] in DATA:
                emit('r[%d] = %#x;' % (rd, DATA[ops[1]])); continue
            if len(ops) < 2 or not ops[1].startswith('['): raise Manual('address form ' + args)
            inner = ops[1]
            post = ops[2] if len(ops) > 2 else None
            wb = inner.endswith('!')
            inner = inner.rstrip('!').strip()[1:-1]
            ia = split_ops(inner)
            rn = REGS[ia[0].lower()]
            off = '0'
            if len(ia) > 1:
                if ia[1].startswith('#'): off = '%d' % expr(ia[1][1:], equs)
                elif ia[1].lstrip('-').lower() in REGS: off = ('-' if ia[1].startswith('-') else '') + 'r[%d]' % REGS[ia[1].lstrip('-').lower()]
                else: raise Manual('offset ' + ia[1])
            byte = bsfx and 'B' in bsfx
            if rn == 13:
                if byte: raise Manual('byte via sp')
                idx = 'sp + (%s) / 4' % off
                if base == 'LDR': emit('r[%d] = stk[%s];' % (rd, idx))
                else: emit('stk[%s] = (uint32_t) r[%d];' % (idx, rd))
                if post:
                    if not post.startswith('#'): raise Manual('sp post-index')
                    # the access happens at the old sp, then sp moves
                    body.pop()
                    k = expr(post[1:], equs)
                    idx0 = 'sp'
                    if base == 'LDR': emit('r[%d] = stk[sp]; sp += %d;' % (rd, k // 4))
                    else: emit('stk[sp] = (uint32_t) r[%d]; sp += %d;' % (rd, k // 4))
                elif wb: raise Manual('sp pre-index writeback')
                continue
            addr = '(r[%d] + (int64_t) (%s))' % (rn, off) if not post else 'r[%d]' % rn
            sz = '8' if byte else '32'
            if base == 'LDR':
                emit('r[%d] = *(volatile uint%s_t *) (uintptr_t) %s;' % (rd, sz, addr))
            else:
                emit('*(volatile uint%s_t *) (uintptr_t) %s = (uint%s_t) r[%d];' % (sz, addr, sz, rd))
            if post:
                if not post.startswith('#'): raise Manual('post-index ' + post)
                emit('r[%d] += %d;' % (rn, expr(post[1:], equs)))
            elif wb: emit('r[%d] += (int64_t) (%s);' % (rn, off))
            continue
        if base in ('LDM', 'STM'):
            m = re.match(r'^(\w+)(!?)\s*,\s*(\{.*\})', args)
            if not m: raise Manual('ldm form ' + args)
            rn = REGS[m.group(1).lower()]; wbk = bool(m.group(2)); regs = parse_regs(m.group(3))
            md = mode or 'IA'
            if rn == 13:
                n = len(regs)
                if base == 'STM' and md in ('FD', 'DB'):
                    st = 'sp -= %d;' % n
                    for k, rg in enumerate(regs): st += ' stk[sp + %d] = (uint32_t) r[%d];' % (k, rg)
                    emit(st)
                elif base == 'LDM' and md in ('FD', 'IA'):
                    ret_here = 15 in regs
                    st = ''
                    for k, rg in enumerate(regs):
                        if rg == 15: continue
                        st += ' r[%d] = stk[sp + %d];' % (rg, k)
                    st += ' sp += %d;' % n
                    if ret_here: st += ' RETURN'
                    emit(st)
                else: raise Manual('stack mode ' + md)
                continue
            if base == 'LDM' and md == 'FD': md = 'IA'
            if base == 'STM' and md == 'FD': md = 'DB'
            if md not in ('IA', 'DB'): raise Manual('ldm mode ' + md)
            first = 0 if md == 'IA' else -4 * len(regs)
            # the transfer addresses come from the value that the base register had before the instruction: if the base is in the list of an LDM, it must not be re-read after it has been loaded
            inlist = base == 'LDM' and rn in regs
            st = ' { uint64_t b_ = r[%d];' % rn if inlist else ''
            for k, rg in enumerate(regs):
                if rg == 15: raise Manual('ldm pc')
                a = ('(b_ + %d)' % (first + 4 * k)) if inlist else '(r[%d] + %d)' % (rn, first + 4 * k)
                st += (' r[%d] = *(volatile uint32_t *) (uintptr_t) %s;' % (rg, a)) if base == 'LDM' else (' *(volatile uint32_t *) (uintptr_t) %s = (uint32_t) r[%d];' % (a, rg))
            if inlist: st += ' }'
            if wbk: st += ' r[%d] += %d;' % (rn, (4 if md == 'IA' else -4) * len(regs))
            emit(st); continue
        raise Manual('instruction ' + mn)
    # assemble the C function
    def ptr(t): return '*' in t
    sig = ', '.join('%s a%d' % (p, i + 1) for i, p in enumerate(params)) if params else 'void'
    c = ['/* generated by tools/armv2c.py from the DeskLib assembler: do not edit */', '#include <stdint.h>', '#include <stddef.h>', '#include "dl64.h"']
    c.append('#undef %s' % name)
    c.append('%s %s (%s)' % (ret, name, sig))
    c.append('{')
    c.append('  uint64_t r[16] = { 0 };')
    c.append('  uint32_t stk[96] = { 0 };')
    c.append('  int Z = 0, N = 0, C = 0, V = 0, sp = %d;' % sp_init)
    c.append('  (void) Z; (void) N; (void) C; (void) V;')
    for i, p in enumerate(params):
        cv = 'DL_ARG (a%d)' % (i + 1)
        if re.search(r'\b(float|double)\b', p): raise Manual('floating point argument')
        if i < 4: c.append('  r[%d] = %s;' % (i, cv))
        else: c.append('  r[%d] = %s; stk[%d] = (uint32_t) %s;' % (i, cv, sp_init + i - 4, cv))   # v1.. also hold them if the code reads them (it does not: they are on the stack)
    # arguments after the fourth live only on the stack: clear the registers v1.. that the above filled
    for i in range(4, nparams): c.append('  r[%d] = 0;' % i)
    if ptr(ret): rexpr = '(%s) (uintptr_t) r[0]' % ret
    elif ret == 'void': rexpr = ''
    else: rexpr = '(%s) (uint32_t) r[0]' % ret
    for st in body: c.append('  ' + st.replace('RETURN', 'return %s;' % rexpr if rexpr else 'return;'))
    c.append('  return %s;' % rexpr if rexpr else '  return;')
    c.append('}')
    if any('**' in p.replace(' ', '') for p in params): REVIEW.append(name)
    return '\n'.join(c) + '\n'

def load_protos(incdir):
    protos = {}
    for h in glob.glob(os.path.join(incdir, '*.h')):
        s = open(h, encoding='latin-1').read()
        s = re.sub(r'/\*.*?\*/', ' ', s, flags=re.S)
        s = re.sub(r'#[^\n]*', ' ', s)
        for m in re.finditer(r'(?:extern\s+)?((?:const\s+|unsigned\s+|struct\s+|signed\s+)*[A-Za-z_]\w*(?:\s*\*+)?)\s*(?:\*+\s*)?\b([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*;', s):
            ret, name, params = m.group(1).strip(), m.group(2), m.group(3).strip()
            # the pointer stars may be glued to the name
            full = s[m.start():m.end()]
            stars = re.match(r'(?:extern\s+)?(?:(?:const|unsigned|struct|signed)\s+)*\w+\s*(\*+)?\s*(\*+)?\s*' + name, full)
            nstar = ''.join(g for g in (stars.groups() if stars else ()) if g) if stars else ''
            if nstar and '*' not in ret: ret = ret + ' ' + nstar
            ps = []
            if params and params != 'void':
                depth, cur = 0, ''
                for ch in params:
                    if ch == '(': depth += 1
                    if ch == ')': depth -= 1
                    if ch == ',' and depth == 0: ps.append(cur.strip()); cur = ''
                    else: cur += ch
                if cur.strip(): ps.append(cur.strip())
            if '...' in ps: continue
            ps2 = []
            for p in ps:
                pm = re.match(r'^(.*?[\s\*])([A-Za-z_]\w*)\s*(\[[^\]]*\])?$', p)
                if pm and pm.group(2) not in ('int', 'char', 'void', 'short', 'long', 'unsigned', 'double', 'float'):
                    t = pm.group(1).strip() + (' *' if pm.group(3) else '')
                else: t = p
                if '(' in t: t = 'void *'      # a function pointer parameter
                ps2.append(t)
            protos[name] = (ret, ps2)
    return protos

def hand_names(src):
    names = set()
    hd = os.path.join(os.path.dirname(os.path.abspath(src)), 'hand')
    for f in glob.glob(os.path.join(hd, '*.c')):
        for m in re.finditer(r'^[A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\s*\(', open(f, encoding='latin-1').read(), re.M): names.add(m.group(1))
    return names

def main():
    src, out = sys.argv[1], sys.argv[2]
    hand = hand_names(src)
    protos = load_protos(os.path.join(src, 'include'))
    protos.update(load_protos(os.path.join(src, 'oldinclude')))
    for k, v in EXTRA.items(): protos.setdefault(k, v)
    allequs = load_equs(glob.glob(os.path.join(src, 'Libraries', '*', 'SwiNos.h')) + [os.path.join(src, 'Libraries', 'RegDefs.h')])
    ok, manual, rep = 0, 0, []
    for s in sorted(glob.glob(os.path.join(src, 'Libraries', '*', '*.s'))):
        lib = os.path.basename(os.path.dirname(s)); stem = os.path.splitext(os.path.basename(s))[0]
        equs = dict(allequs); equs.update(load_equs([s]))
        try:
            funcs = translate(s, equs, protos)
            if not funcs: raise Manual('no STARTCODE')
            def returns(f):
                ins = [it for it in f['items'] if it[0] == 'ins']
                if not ins: return False
                _, mn, args = ins[-1]
                base, cond, sflag, bsfx, mode = parse_mn(mn)
                if cond not in (None, '', 'AL'): return False
                a = args.replace(' ', '').lower()
                if base == 'MOV' and a.startswith('pc,'): return True
                if base == 'LDM' and a.startswith('sp!,') and ('pc}' in a or ',pc' in a): return True
                return False
            for i in range(len(funcs) - 1, -1, -1):
                f = funcs[i]
                if not returns(f) and i + 1 < len(funcs) and not any(it[0] == 'label' for it in f['items']):
                    f['items'] = f['items'] + funcs[i + 1]['items']
            texts = []
            for f in funcs:
                if f['name'] in hand: continue
                texts.append(emit_function(f, equs, protos))
            if not texts: continue
            os.makedirs(os.path.join(out, lib), exist_ok=True)
            hdrs = sorted(set(re.findall(r'#include\s+"DeskLib:\w+\.h"', open(s, encoding='latin-1').read())))
            with open(os.path.join(out, lib, stem + '.c'), 'w') as fh: fh.write('\n'.join(texts))
            ok += 1
        except Manual as e:
            manual += 1; rep.append('%s/%s: %s' % (lib, stem, e))
    with open(os.path.join(out, 'report.txt'), 'w') as fh: fh.write('\n'.join(rep) + '\n')
    with open(os.path.join(out, 'review.txt'), 'w') as fh: fh.write('\n'.join(sorted(set(REVIEW))) + '\n')
    print('translated %d, manual %d' % (ok, manual))

if __name__ == '__main__': main()
