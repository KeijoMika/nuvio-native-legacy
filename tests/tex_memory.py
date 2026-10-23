import pathlib
import re
import subprocess

# Mechanism bisect: shared-context submission + GPU-wide completion wait.
revisions = ['c2e7d0ef9', '9756ff9e9', '44014cf78', '38e369735', 'd8b60bef5', 'e06c70bc8']
first = None
for rev in revisions:
    source = subprocess.check_output(['git', 'show', f'{rev}:src/tex_cache.c'], text=True)
    bad = 'eglCreateContext' in source and 'glFinish();' in source
    print(f'{rev}: shared-context contention mechanism {"present" if bad else "absent"}')
    if bad and first is None:
        first = rev
assert first == '44014cf78'
source = pathlib.Path('src/tex_cache.c').read_text()
assert 'eglCreateContext' not in source, 'shared upload context retained'
assert 'decodeReservado' in source, 'decode bytes not reserved before allocation'
android = pathlib.Path('src/jpegrapido.c').read_text()
assert 'inSampleSize' in android, 'Android decode still depends on optional libjpeg'
# Local PNG/GIF/WebP decode must survive a BitmapFactory miss on Android:
# inside the decode body the SDL/webp/gif fallbacks must not be gated out.
cache = pathlib.Path('src/tex_cache.c').read_text()
dec = cache[cache.index('jpeg_rapido_carregar_mem(bruto'):cache.index('tLoad = SDL_GetTicks()')]
for pat in ['IMG_Load_RW', 'webp_carregar_larg_mem', 'gifFotoMem',
            'IMG_Load(caminho)', 'webp_carregar_larg(caminho', 'gifFotoArquivo']:
    assert pat in dec, f'missing decode fallback: {pat}'
for m in re.finditer(r'#if[^\n]*NV_ANDROID[^\n]*', dec):
    assert 'ifndef' not in m.group(0), m.group(0)
print('PASS: mechanism bisect and bounded Android decode wiring')
