#!/usr/bin/env python3
"""Render verbatim log excerpts, explicitly not desktop screenshots. Needs Pillow."""
import pathlib
import re
from PIL import Image, ImageDraw, ImageFont

report = pathlib.Path(__file__).resolve().parents[2] / 'report'
fonts = [pathlib.Path('C:/Windows/Fonts/consola.ttf'),
         pathlib.Path('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf')]
font_path = next(path for path in fonts if path.exists())
font = ImageFont.truetype(str(font_path), 24)
small = ImageFont.truetype(str(font_path), 19)
heading = ImageFont.truetype(str(font_path), 29)

def read(name):
    return (report/name).read_text(encoding='utf-8', errors='replace').splitlines()

def render(name, title, source, lines):
    width = max(1280, max((int(font.getlength(line)) for line in lines), default=0)+80)
    height = 138 + 34*len(lines)
    im = Image.new('RGB', (width, height), '#111827')
    draw = ImageDraw.Draw(im)
    draw.text((32, 22), title, font=heading, fill='#e5e7eb')
    draw.text((32, 65), 'LOG EXCERPT / NOT A DESKTOP SCREENSHOT', font=small, fill='#fbbf24')
    draw.text((32, 90), 'Source: '+source, font=small, fill='#9ca3af')
    for i, line in enumerate(lines):
        color = '#86efac' if any(word in line for word in ('PASS', 'OK', '20/20', '6 / 6', 'exit code: 0')) else '#e5e7eb'
        draw.text((32, 125+i*34), line, font=font, fill=color)
    im.save(report/'images'/name)

render('01-build-grade.png', 'Lab2 - Build and course grading',
       'build.log / best-fit-test.log / grade.log',
       read('build.log') + [''] + [x for x in read('best-fit-test.log') if x.startswith('grading:')]
       + [''] + [x for x in read('grade.log') if 'check page init:' in x or 'check best fit:' in x or 'Total Score:' in x])
render('02-page-init.png', 'Lab2 - page_init boundary checks', 'page-init-boundaries.log',
       read('page-init-boundaries.log'))
render('03-buddy-slub.png', 'Lab2 - Buddy and SLUB / 128 MiB', 'buddy-128M.log',
       [x for x in read('buddy-128M.log') if any(t in x for t in
        ('QEMU:', 'memory management:', 'physical memory:', 'free memory:', '[Buddy]', '[SLUB]', 'check_alloc_page'))])
trace = read('mmu-gdb.log')
render('04-mmu-trace.png', 'Lab2 - QEMU page walk and TLB fill', 'mmu-gdb.log',
       [x for x in trace if x.startswith('===') or re.match(r'\$(?:[1-9]|[12]\d|3[0-2]) =', x)
        or x == 'MMU TRACE COMPLETED'])
print('Rendered 4 labeled log figures.')
