"""Original DB2 badge vector geometry. MIT for this generator and new geometry only.
SEGA character identities/trade dress remain their owners'. No source pixels used.
Run from repository root: python assets/achievements/db2/generate_badges.py
Requires resvg-py0.5.0; no font/ROM/network dependency.
"""
import json, pathlib, hashlib
import resvg_py
ROOT=pathlib.Path(__file__).resolve().parent
CAT=ROOT.parent/'db2.json'
INK='#162534'
SOURCES=['https://vc.sega.jp/vc_dyna2/','https://www.sega.jp/mdmini/manual/pdf/m_jp_dyna-brothers2.pdf']
def path(d,fill,stroke=INK,sw=1.6):return f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round"/>'
def ellipse(x,y,rx,ry,c,stroke=INK,sw=1.5):return f'<ellipse cx="{x}" cy="{y}" rx="{rx}" ry="{ry}" fill="{c}" stroke="{stroke}" stroke-width="{sw}"/>'
def eye(x,y,angry=False):
 return ellipse(x,y,4,6,'#fff7d8')+ellipse(x+1,y+1,1.5,3,'#132435',sw=0)+ellipse(x+1,y,0.55,1,'white',sw=0)+(path(f'M{x-4} {y-5}l7 2','none') if angry else '')
def portrait(name):
 if name in ('tyranno','allo'):
  c='#7774df' if name=='tyranno' else '#fa9c36'; shade='#474c9b' if name=='tyranno' else '#c96628'
  p=path('M15 49Q11 40 18 34L24 30 21 18Q21 11 31 11L45 16Q54 20 52 31L46 37 49 49Z',c)
  p+=path('M18 40L12 44 14 49 25 49 30 39Z',shade)
  p+=path('M28 38Q39 36 44 42L43 49 28 49Z','#f9e8a6')
  p+=path('M30 12L34 7 38 14 43 12 46 18', '#f4bd50')
  p+=eye(29,24,name=='tyranno')+eye(41,25,name=='tyranno')
  p+=path('M23 31Q35 28 49 31L48 37Q34 44 23 36Z','#fff1bb')
  p+=path('M28 35Q39 39 46 34','none')
  if name=='tyranno':p+=path('M44 35L42 39 40 36','#ffffff')
  return p
 if name=='ptera':
  p=path('M27 30L12 24 7 45 27 40M39 30L52 24 58 45 39 40','#809de7')
  p+=path('M12 24L24 37M52 24L43 37','none','#46577e',1)
  p+=path('M25 19L13 8 33 14 43 17 48 25 44 34 41 49 28 49 25 35Z','#81bde5')
  p+=path('M31 33Q36 31 41 35L38 48 30 48Z','#fff1b5')
  p+=eye(32,21)+eye(41,22)
  p+=path('M30 27L51 29 41 34 29 32Z','#f6e5a2')
  return p
 if name=='pchan':
  p=path('M23 48L18 39 21 25 26 15 33 12 43 15 46 24 39 30 39 45 44 49Z','#75c5ca')
  p+=path('M27 31Q33 29 37 33L36 48 26 48Z','#fff2b9')+eye(29,20)+eye(39,20)
  p+=path('M25 25Q39 26 47 23L45 31Q35 39 27 30Z','#fff2b9')
  p+=path('M31 29Q37 33 42 28L39 33 33 32Z','#b74349')
  p+=path('M20 38L12 35 11 31 16 29 23 33','#75c5ca')
  p+=ellipse(13,28,5,7,'#ead394')+path('M10 25L12 23 14 26M14 29L16 31M10 31L12 32','none','#a8793c',1)
  return p
 if name=='tricera':
  p=path('M12 36L10 19 17 10 25 8 33 10 40 8 50 17 53 34 46 46 19 47Z','#df7e28')
  p+=path('M16 25L13 14 23 22 30 12 36 23 48 15 44 28','#f5ac48')
  p+=path('M17 30L19 21 24 17 26 29M38 28L39 15 44 11 44 29','#fff3cf')
  p+=path('M17 34Q16 25 31 26 49 24 49 38L45 48 18 48Z','#ee9b38')
  p+=eye(26,33,True)+eye(39,33,True)
  p+=path('M23 41Q35 38 47 39L44 47 25 46Z','#ffe4a0')
  p+=path('M33 39L36 29 39 40Z','#fff3cf')
  return p
 if name=='stego':
  p=''
  for x,y in [(16,19),(24,12),(34,9),(44,13),(51,22)]:p+=path(f'M{x-5} {y+13}L{x-6} {y+2} {x} {y-3} {x+6} {y+2} {x+4} {y+14}Z','#e8a536')
  p+=path('M12 43Q12 25 28 22 48 17 53 35L49 48 18 49Z','#8bb850')
  p+=path('M14 31Q24 23 34 31L36 43 30 48 13 47 9 40Z','#a8cf63')
  p+=eye(18,34)+eye(28,33)
  p+=path('M11 41Q23 39 33 40L31 47 16 48Z','#fff0b3')+path('M17 44Q23 47 28 43','none')
  p+=path('M40 29L44 28M44 36L48 35','none','#e5b655',2)
  return p
 raise ValueError(name)
CHARS={0:'stego',1:'tyranno',3:'allo',4:'ptera',5:'pchan',6:'tricera'}
ACCENTS={0:'#83c65f',1:'#9788ee',3:'#ffb244',4:'#7bc2f3',5:'#6bd4cc',6:'#ea8d40'}
GLYPHS={'0':['111','101','101','101','111'],'1':['010','110','010','010','111'],'2':['111','001','111','100','111'],'3':['111','001','111','001','111'],'4':['101','101','111','001','001'],'5':['111','100','111','001','111'],'6':['111','100','111','101','111'],'7':['111','001','010','010','010'],'8':['111','101','111','101','111'],'9':['111','101','111','001','111']}
def digits(s,x,y,color):
 return ''.join(f'<rect x="{x+j*8+xx*2}" y="{y+yy*2}" width="2" height="2" fill="{color}"/>' for j,c in enumerate(s) for yy,row in enumerate(GLYPHS[c]) for xx,v in enumerate(row) if v=='1')
def svg(mode,stage=None,locked=False):
 name=CHARS[mode];accent=ACCENTS[mode];complete=stage is None
 art=portrait(name)
 s='<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64"><defs><clipPath id="portrait"><rect x="5" y="5" width="54" height="44" rx="4"/></clipPath><filter id="mono"><feColorMatrix type="saturate" values="0"/></filter></defs>'
 s+=f'<g'+(' filter="url(#mono)"' if locked else '')+'>'
 s+='<rect x="1" y="1" width="62" height="62" rx="6" fill="#101c2d"/>'
 s+=f'<rect x="2.5" y="2.5" width="59" height="59" rx="4.5" fill="none" stroke="{accent if not complete else "#f6d372"}" stroke-width="2"/>'
 s+='<rect x="5" y="5" width="54" height="44" rx="3" fill="#243d52"/>'
 s+=path('M5 40L20 20 34 39 46 17 59 36 59 49 5 49Z','#315468','none')
 s+=f'<g clip-path="url(#portrait)">{art}</g>'
 s+=f'<path d="M6 50H58" stroke="{accent}" stroke-width="1"/>'
 if complete:s+=path('M25 55L29 59 39 52','none','#f6d372',3)
 else:s+=digits(f'{stage:02}',25,52,'#fff0c5')
 # Roman-like mode markers: Practice leaf, Story diamond, Original I-IV bars.
 if mode==0:s+=path('M9 58Q8 52 16 52Q16 58 9 58Z',accent,'none')
 elif mode==1:s+=path('M12 52L16 56 12 60 8 56Z',accent,'none')
 else:
  for j in range(mode-2):s+=f'<rect x="{8+j*3}" y="53" width="2" height="6" fill="{accent}"/>'
 if locked:s+='<rect x="50" y="53" width="7" height="7" rx="1" fill="#d4d8dd"/><path d="M51 53V51a2.5 2.5 0 0 1 5 0v2" fill="none" stroke="#d4d8dd" stroke-width="1.5"/>'
 return s+'</g></svg>'
def main():
 d=json.loads(CAT.read_text(encoding='utf-8'));before=json.loads(json.dumps(d));files=[]
 for mode,name in CHARS.items():(ROOT/f'portrait_{name}.svg').write_text(svg(mode),encoding='utf-8')
 for a in d['achievements']:
  for locked in (False,True):
   name=f'{a["id"]}'+('_lock' if locked else '')+'.png';xml=svg(a['mode'],a.get('stage') if a['kind']=='stage' else None,locked)
   data=resvg_py.svg_to_bytes(svg_string=xml);(ROOT/name).write_bytes(data)
   a['badge_file_locked' if locked else 'badge_file_unlocked']='db2/'+name
   files.append(dict(path=name,sha256=hashlib.sha256(data).hexdigest(),bytes=len(data)))
 # Preserve every catalog field except the requested paths.
 for old,new in zip(before['achievements'],d['achievements']):
  for k,v in old.items():
   if k not in ('badge_file_locked','badge_file_unlocked'):assert new[k]==v
 CAT.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 manifest=dict(art_type='Original code-native SVG interpretations; no copied source pixels, ROM or DB1 badge assets',canvas=[64,64],portrait_palette='Consistent ink, cream highlight, species colors informed by official references; upper-left lighting',characters=CHARS,mode_mapping='Practice:Stego; Story:Tyranno; Original I:Allo; II:Ptera; III:P-chan; IV:Tricera',source_urls=SOURCES,reference_images=[f'https://vc.sega.jp/vc_dyna2/chara{i}.jpg' for i in range(1,7)],license='MIT for generator and newly authored vector geometry; SEGA character identities and trademarks remain their owners. Does not license the reference artwork.',rasterizer='resvg-py0.5.0',achievement_count=len(d['achievements']),file_count=len(files),files=files)
 (ROOT/'provenance.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 print(f'Generated {len(files)} PNGs for {len(d["achievements"])} achievements;6SVG portraits.')
if __name__=='__main__':main()
