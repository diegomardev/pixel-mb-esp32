#!/usr/bin/env python3
"""AWTRIXER/Piskel v2 -> JSON del editor y/o AWX1. Requiere Pillow."""
import argparse,base64,io,json
from pathlib import Path
from PIL import Image
from awx import encode,validate

def convert(document):
    if document.get('modelVersion')!=2:raise ValueError('Solo AWTRIXER/Piskel modelVersion 2')
    p=document['piskel'];w,h=p['width'],p['height'];fps=p['fps']
    if type(w)!=int or type(h)!=int or not 1<=w<=32 or not 1<=h<=8 or not isinstance(fps,(int,float)) or not 0.2<=fps<=50:raise ValueError('Dimensiones/FPS incompatibles')
    layers=[json.loads(v) if isinstance(v,str) else v for v in p['layers']]
    if not 1<=len(layers)<=16:raise ValueError('Capas: 1..16')
    count=layers[0]['frameCount']
    if type(count)!=int or not 1<=count<=120:raise ValueError('Fotogramas: 1..120')
    frames=[Image.new('RGBA',(w,h),(0,0,0,255)) for _ in range(count)]
    # Piskel guarda las capas de arriba hacia abajo.
    for layer in reversed(layers):
        if layer['frameCount']!=count:raise ValueError('Capas con distinta longitud')
        opacity=layer.get('opacity',1)
        if not isinstance(opacity,(int,float)) or not 0<=opacity<=1:raise ValueError('Opacidad invalida')
        seen=set()
        for chunk in layer['chunks']:
            src=chunk['base64PNG']
            if not src.startswith('data:image/png;base64,'):raise ValueError('Se espera PNG incrustado')
            with Image.open(io.BytesIO(base64.b64decode(src.split(',',1)[1],validate=True))) as image:
                if image.width>4096 or image.height>4096:raise ValueError('Imagen excesiva')
                image=image.convert('RGBA')
                for x,column in enumerate(chunk['layout']):
                    for y,index in enumerate(column):
                        if index is None or index==-1:continue
                        if type(index)!=int or not 0<=index<count or index in seen or (x+1)*w>image.width or (y+1)*h>image.height:raise ValueError('Layout invalido')
                        seen.add(index);tile=image.crop((x*w,y*h,(x+1)*w,(y+1)*h))
                        if opacity!=1:tile.putalpha(tile.getchannel('A').point(lambda a:round(a*opacity)))
                        frames[index]=Image.alpha_composite(frames[index],tile)
        if len(seen)!=count:raise ValueError('Faltan fotogramas')
    return validate(dict(width=w,height=h,frameMs=round(1000/fps),frames=[['#%02x%02x%02x'%pixel[:3] for pixel in frame.getdata()] for frame in frames]))
if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__);cli.add_argument('input',type=Path);cli.add_argument('output',type=Path);args=cli.parse_args()
    result=convert(json.loads(args.input.read_text(encoding='utf-8-sig')))
    if args.output.suffix.lower()=='.awx':args.output.write_bytes(encode(result))
    else:args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(f"{result['width']}x{result['height']}, {len(result['frames'])} fotogramas, {result['frameMs']} ms")
