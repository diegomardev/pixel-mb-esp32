#!/usr/bin/env python3
"""Añade JSON legible a la biblioteca, conservando los AWX compactos."""
import json
from pathlib import Path
from awx import decode,encode
ROOT=Path(__file__).resolve().parents[1];LIB=ROOT/'library'
def export_other_json(item):
    from PIL import Image
    frames=[];durations=[]
    with Image.open(LIB/item['preview']) as image:
        for index in range(getattr(image,'n_frames',1)):
            image.seek(index);image.load()
            rgba=image.convert('RGBA').resize((item['width'],item['height']),Image.Resampling.NEAREST)
            base=Image.new('RGBA',rgba.size,'black');base.alpha_composite(rgba)
            frames.append(['#%02x%02x%02x'%color for color in base.convert('RGB').getdata()])
            durations.append(max(20,int(image.info.get('duration',100))))
    count=min(120,len(frames));total=sum(durations);output=[];index=0;end=durations[0]
    for n in range(count):
        at=n*total/count
        while index<len(frames)-1 and at>=end:index+=1;end+=durations[index]
        output.append(frames[index])
    animation=dict(name=item['title'],source=item.get('source',''),note=item.get('quality','')+' · No compatible con ESP 32x8',width=item['width'],height=item['height'],frameMs=max(20,round(total/count)),frames=output)
    text=json.dumps(animation,ensure_ascii=False,separators=(',',':'))+'\n'
    path=Path('animations')/(item['id']+'.json');(LIB/path).write_text(text)
    item['json']=path.as_posix();item['jsonBytes']=len(text.encode())

def export_json(item):
    if not item.get('compatible'):return export_other_json(item)
    data=(LIB/item['awx']).read_bytes();animation=decode(data)
    header=dict(name=item['title'],source=item.get('source',''),note=item.get('quality',''),width=animation['width'],height=animation['height'],frameMs=animation['frameMs'])
    text=json.dumps(header,ensure_ascii=False,indent=2)[:-2]+',\n  "frames": [\n'
    text+=',\n'.join('    '+json.dumps(frame,separators=(',',':')) for frame in animation['frames'])+'\n  ]\n}\n'
    parsed=json.loads(text);assert encode(parsed)==data
    if len(text.encode())>500000:raise ValueError('JSON demasiado grande para el importador')
    path=Path(item['awx']).with_suffix('.json');(LIB/path).write_text(text,encoding='utf-8')
    item['json']=path.as_posix();item['jsonBytes']=len(text.encode())
if __name__=='__main__':
    path=LIB/'catalog.json';catalog=json.loads(path.read_text())
    for item in catalog['items']:
        if item['status']=='ready':export_json(item)
    tmp=path.with_suffix('.tmp');tmp.write_text(json.dumps(catalog,ensure_ascii=False,separators=(',',':'))+'\n');tmp.replace(path)
    print('JSON generados:',sum('json' in i for i in catalog['items']))
