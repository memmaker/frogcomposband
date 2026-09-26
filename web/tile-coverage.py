#!/usr/bin/env python3
"""Coverage of FrogComposband's own 16x16 set (graf-new.prf + xtra-new.prf +
16x16.bmp) over every monster (r_info), object (k_info) and feature (f_info)
entry. Monsters/features by index, objects by I:tval:sval (k_info numbers
with N:*, the pref uses K:tval:sval). A mapping counts when it points to a
tile inside the bitmap."""
import re, sys
from PIL import Image
L = sys.argv[1] if len(sys.argv) > 1 else 'lib'
W, H = Image.open(f'{L}/xtra/graf/16x16.bmp').size
ok = lambda a, c: a & 0x80 and c & 0x80 and (c & 0x7F) * 16 + 16 <= W and (a & 0x7F) * 16 + 16 <= H
maps = {}
for p in ('graf-new', 'xtra-new'):
    for line in open(f'{L}/pref/{p}.prf', encoding='latin-1'):
        m = re.match(r'([RF]):(\d+):(0x\w+)/(0x\w+)', line) or re.match(r'(K):(\d+:\d+):(0x\w+)/(0x\w+)', line)
        if m:
            maps[(m[1], m[2])] = ok(int(m[3], 16), int(m[4], 16))
tot = hit = 0
for kind, f in (('R', 'r_info'), ('K', 'k_info'), ('F', 'f_info')):
    txt = open(f'{L}/edit/{f}.txt', encoding='latin-1').read()
    if kind == 'K':
        ids = [f'{int(t)}:{int(s)}' for t, s in re.findall(r'^I:(\d+):(\d+)', txt, re.M)]
    else:
        ids = re.findall(r'^N:(\d+):', txt, re.M)
    miss = [i for i in ids if not maps.get((kind, i))]
    print(f'{f}: {len(ids) - len(miss)}/{len(ids)}  missing: {miss[:20]}{" ..." if len(miss) > 20 else ""}')
    tot += len(ids); hit += len(ids) - len(miss)
print(f'total: {hit}/{tot} = {100 * hit / tot:.1f}%')
