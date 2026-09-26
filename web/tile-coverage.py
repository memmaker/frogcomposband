#!/usr/bin/env python3
"""Coverage of a tile set over every monster (r_info), object (k_info) and
feature (f_info) entry.  Monsters/features by index, objects by I:tval:sval
(k_info numbers with N:*, the pref uses K:tval:sval).  An entry counts when
the pref maps it to a non-empty tile inside the sheet.

  python3 web/tile-coverage.py         Shockbolt (graf-shb.prf, 64x64)
  python3 web/tile-coverage.py new     own 16x16 set (graf-new.prf + xtra-new.prf)"""
import re, sys, os
from PIL import Image
L = 'lib'
if sys.argv[1:] == ['new']:
    prefs, sheet, S = ('graf-new', 'xtra-new'), f'{L}/xtra/graf/16x16.bmp', 16
else:
    prefs, sheet, S = ('graf-shb',), os.path.expanduser('~/Games/tactical-angband/lib/tiles/shockbolt/64x64.png'), 64
img = Image.open(sheet).convert('RGBA')
W, H = img.size
def ok(a, c):
    x, y = (c & 0x7F) * S, (a & 0x7F) * S
    if not (a & 0x80 and c & 0x80) or x + S > W or y + S > H:
        return False
    # plain one-colour tiles count (16x16 floor is plain grey); empty = fully transparent
    return img.crop((x, y, x + S, y + S)).getbbox() is not None
maps, stand = {}, set()
for p in prefs:
    prev = ''
    for line in open(f'{L}/pref/{p}.prf', encoding='latin-1'):
        m = re.match(r'([RF]):(\d+):(0x\w+)/(0x\w+)', line) or re.match(r'(K):(\d+:\d+):(0x\w+)/(0x\w+)', line)
        if m:
            maps[(m[1], m[2])] = ok(int(m[3], 16), int(m[4], 16))
            if prev.endswith('(stand-in)'): stand.add((m[1], m[2]))
        prev = line.rstrip()
tot = hit = 0
for kind, f in (('R', 'r_info'), ('K', 'k_info'), ('F', 'f_info')):
    txt = open(f'{L}/edit/{f}.txt', encoding='latin-1').read()
    if kind == 'K':
        ids = [f'{int(t)}:{int(s)}' for t, s in re.findall(r'^I:(\d+):(\d+)', txt, re.M)]
    else:
        ids = re.findall(r'^N:(\d+):', txt, re.M)
    miss = [i for i in ids if not maps.get((kind, i))]
    si = sum((kind, i) in stand for i in ids if i not in miss)
    print(f'{f}: {len(ids) - len(miss)}/{len(ids)} ({si} family stand-ins)  missing: {miss[:20]}{" ..." if len(miss) > 20 else ""}')
    tot += len(ids); hit += len(ids) - len(miss)
print(f'total: {hit}/{tot} = {100 * hit / tot:.1f}%, family stand-ins: {len(stand)}')
