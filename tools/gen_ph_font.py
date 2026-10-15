#!/usr/bin/env python3
# tools/gen_ph_font.py —— 从口袋历史源码抽取字集,用 lv_font_conv 生成 app_font_16/20 子集字库。
# 用法: python tools/gen_ph_font.py  (仓库根目录运行)
# 字源:本机 NotoSansSC-VF.ttf(SIL OFL)。改为发布用时,把字体归档进 assets/fonts 并更新 FONT_SRC。
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NODE = r"C:\Users\Administrator\.workbuddy\binaries\node\versions\22.22.2-6\node.exe"
LFC  = os.path.join(r"C:\Users\Administrator\.workbuddy\binaries\node\workspace",
                    "node_modules", "lv_font_conv", "lv_font_conv.js")
FONT_SRC = r"C:\Windows\Fonts\NotoSansSC-VF.ttf"
OUT_DIR = os.path.join(ROOT, "assets", "fonts")
SOURCES = ["main/pocket_history_data.h", "main/demo_pocket_history.c"]

WANTED = [(0x20,0x7E),(0x00A0,0x00FF),(0x0370,0x03FF),(0x2000,0x206F),
          (0x2190,0x21FF),(0x2200,0x22FF),(0x3000,0x303F),(0x4E00,0x9FFF),(0xFF00,0xFFEF)]

def wanted(cp):
    return any(lo<=cp<=hi for lo,hi in WANTED)

def strip_and_collect(text, acc):
    i, n = 0, len(text)
    state = 'N'  # N normal, S string, C char, B block-comment, L line-comment
    while i < n:
        c = text[i]; nxt = text[i+1] if i+1<n else ''
        if state=='N':
            if c=='/' and nxt=='*': state='B'; i+=2; continue
            if c=='/' and nxt=='/': state='L'; i+=2; continue
            if c=='"': state='S'; i+=1; continue
            if c=="'": state='C'; i+=1; continue
            cp=ord(c)
            if wanted(cp): acc.add(cp)
        elif state=='S':
            if c=='\\': i+=2; continue
            if c=='"': state='N'; i+=1; continue
            cp=ord(c)
            if wanted(cp): acc.add(cp)
        elif state=='C':
            if c=='\\': i+=2; continue
            if c=="'": state='N'; i+=1; continue
        elif state=='B':
            if c=='*' and nxt=='/': state='N'; i+=2; continue
        elif state=='L':
            if c=='\n': state='N'
        i+=1

def merge_ranges(cps):
    cps = sorted(c for c in cps if c>0x7E)   # ASCII 单独一段
    out=[]; s=p=None
    for c in cps:
        if s is None: s=p=c
        elif c==p+1: p=c
        else: out.append((s,p)); s=p=c
    if s is not None: out.append((s,p))
    return out

def main():
    acc=set()
    for rel in SOURCES:
        path=os.path.join(ROOT, rel)
        with open(path, encoding='utf-8') as f:
            strip_and_collect(f.read(), acc)
    os.makedirs(OUT_DIR, exist_ok=True)
    sym=os.path.join(OUT_DIR, "ph_font_symbols.txt")
    with open(sym,'w',encoding='utf-8') as f:
        f.write(''.join(chr(c) for c in sorted(acc)))
    ranges=[(0x20,0x7E)]+merge_ranges(acc)
    rargs=[]
    for lo,hi in ranges:
        rargs += ["--range", f"0x{lo:X}-0x{hi:X}"]
    print(f"字集码点:{len(acc)}  合并range段:{len(ranges)}")
    for size in (16,20):
        name=f"app_font_{size}"
        outp=os.path.join(OUT_DIR, f"{name}.c")
        args=[NODE, LFC, "--font", FONT_SRC, "--size", str(size), "--bpp","4",
              "--format","lvgl","--no-compress","--lv-include","lvgl.h",
              "--lv-font-name", name] + rargs + ["--output", outp]
        print("生成", name, "...")
        r=subprocess.run(args, capture_output=True, text=True)
        if r.returncode!=0:
            print("lv_font_conv FAILED:", r.stdout[-2000:], r.stderr[-2000:]); sys.exit(1)
        # 归一化 include:lvgl/lvgl.h -> lvgl.h(仓库用 LV_CONF_INCLUDE_SIMPLE)
        with open(outp, encoding='utf-8') as f: src=f.read()
        src2=src.replace('#include "lvgl/lvgl.h"', '#include "lvgl.h"')
        if src2!=src:
            with open(outp,'w',encoding='utf-8') as f: f.write(src2)
            print(f"  [fix] {name}.c include 归一化为 lvgl.h")
        print(f"  -> {outp}  ({os.path.getsize(outp)} bytes)")
    print("字表已存:", sym)

if __name__=="__main__":
    main()
