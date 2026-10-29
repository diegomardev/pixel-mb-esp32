#!/usr/bin/env python3
"""Adaptaciones AWTRIX: v1 CC BY 4.0 (Blueforcer), v2 BSD-2-Clause (Stephan Mühl).
Ver ANIMACIONES.md y licenses/. Genera secuencias finitas AWX sin servidor.
"""
import json,random,re,struct
from pathlib import Path
from awx import encode,decode
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'tools/awtrix_sources'
def save(name,frames,ms):
    data=struct.pack('<4sBBHHH',b'AWX1',32,8,len(frames),ms,0)+b''.join(struct.pack('<256H',*f) for f in frames)
    (ROOT/'data/animations'/f'{name}.awx').write_bytes(data)
    (ROOT/'examples'/f'{name}.json').write_text(json.dumps(decode(data),separators=(',',':'))+'\n')
    assert encode(decode(data))==data
    print(name,len(frames),'frames',len(data),'bytes')
def rgb(r,g,b):return (max(0,min(255,r))>>3)<<11 | (max(0,min(255,g))>>2)<<5 | max(0,min(255,b))>>3
# FireApp.cpp v1: misma difusión de cuatro vecinos /7.5. Semilla reproducible;
# 60 fotogramas a 50 ms; base aleatoria cada 100 ms; canales acotados a 255.
rng=random.Random(10);fire=[[0]*32 for _ in range(8)];frames=[]
for t in range(80):
    if t%2==0:fire[7]=[rng.randrange(1024) for _ in range(32)]
    for y in range(7):
        for x in range(32):fire[y][x]=int((fire[y+1][(x-1)%32]+fire[y+1][x]+fire[y+1][(x+1)%32]+fire[(y+2)%8][x])/7.5)
    if t>=20:frames.append([rgb(v,0,0) if v<256 else rgb(255,v-256,0) for row in fire for v in row])
save('aw1_fire',frames,50)
# GolApp.cpp v1: toro 32x8; nacimientos verdes, supervivientes azules.
# Inicio determinista con 85 células; secuencia finita de 48 generaciones.
alive=set(rng.sample(range(256),85));frames=[]
for t in range(48):
    next_alive=set();frame=[0]*256
    for y in range(8):
        for x in range(32):
            i=y*32+x;n=sum(((y+dy)%8)*32+(x+dx)%32 in alive for dy in [-1,0,1] for dx in [-1,0,1] if dx or dy)
            if n==3 or (i in alive and n==2):next_alive.add(i);frame[i]=rgb(0,0,255) if i in alive else rgb(0,255,0)
    frames.append(frame);alive=next_alive
save('aw1_life',frames,500)
# Matrix.bas v2: copia exacta de las 35 matrices RGB565 y App.Tick=100.
text=(SOURCE/'v2-matrix.bas').read_text(encoding='utf-8-sig')
frames=[[int(v.strip())&65535 for v in row.split(',')] for row in re.findall(r'Dim Frame\d+\(\) As Short = Array As Short\(([^)]+)\)',text)]
assert len(frames)==35 and all(len(f)==256 for f in frames)
save('aw2_matrix',frames,100)
# LookingEyes.bas v2: sprites originales n0..n4, ojos x=6/18.
# Trayectoria y parpadeo fijados para una secuencia repetible de 64 frames.
text=(SOURCE/'v2-eyes.bas').read_text(encoding='utf-8-sig')
eyes=[[int(v.strip())&65535 for v in row.split(',')] for row in re.findall(r'Dim n\d+\(\) As Short = Array As Short\(([^)]+)\)',text)]
assert len(eyes)==5 and all(len(f)==64 for f in eyes)
frames=[]
for t in range(64):
    blink={26:1,27:2,28:3,29:4,30:3,31:2,32:1}.get(t,0)
    pupil=[(3,3),(1,3),(4,2),(3,3)][t//16]
    frame=[0]*256
    for origin in [6,18]:
        for y in range(8):
            for x in range(8):frame[y*32+origin+x]=eyes[blink][y*8+x]
        if blink==0:
            for y in range(pupil[1],pupil[1]+2):
                for x in range(pupil[0],pupil[0]+2):frame[y*32+origin+x]=0
    frames.append(frame)
save('aw2_eyes',frames,90)
