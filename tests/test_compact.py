"""Comprueba AWX1/AWX2 reales y genera datos para el test del editor JS."""
from pathlib import Path
import sys,json,base64
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from awx import encode,encode_compact,decode
def main():
    out=[]
    for p in sorted((root/'examples').glob('*.json')):
        a=json.loads(p.read_text());raw=encode(a);compact=encode_compact(a)
        assert encode(decode(compact))==raw,p.name
        for bad in [compact[:-1],compact+b'x']:
            try:decode(bad)
            except ValueError:pass
            else:raise AssertionError('Se aceptaron datos inválidos: '+p.name)
        out.append(dict(source=p.name,compact=base64.b64encode(compact).decode()))
    target=Path(sys.argv[1]) if len(sys.argv)>1 else root/'work/compact-fixture.json'
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps(out))
    print('Python AWX1/AWX2: '+str(len(out))+' archivos sin pérdida; se rechazan datos truncados y sobrantes')

if __name__=='__main__':
    main()
