"""Guarded, data-only DB1 P-chan speech translation; no pipeline integration.

The original eight 32x32 frames retain their bubble, actor, palette and animation.
Only prequalified interior masks change. Source must be the current guarded
English image; CLI outputs are exclusive new files beneath this project's local/.
"""
from dataclasses import dataclass
import argparse
import hashlib
import json
from pathlib import Path

SOURCE_SHA256='ad9a90450d46b860dfd38772fa19655576e4244bf7c4df4c2fdb8d7786c1c261'
@dataclass(frozen=True)
class SpeechFrame:
    frame:int
    address:int
    capacity:int
    guard:str
    text:str
    ink:int
    bounds:tuple

FRAMES=(
 SpeechFrame(0x1e,0x8f5c6,301,'163db3e2f3de1c2847afaef22a39ec7e2409a4d44e258a54b8b8b4e7b1eaac6d','STU',15,((3,23),(2,24),(1,25),(0,25),(0,25),(0,25),(0,25),(0,25),(0,25),(0,24),(1,23))),
 SpeechFrame(0x1f,0x8f6f3,308,'36eb6a7786bddbb951495b56f277269c6d3b4b7cc9ab7e971d3bcd3275409644','PID!',15,((8,28),(7,29),(6,30),(6,31),(6,31),(6,31),(6,31),(6,31),(6,31),(7,31),(8,30))),
 SpeechFrame(0x27,0x8ffb1,328,'b5edb461e03aa0cf40506c5c753cfca54b3728377bab35b2c7668cd6feae73a1','NYAH',15,((3,26),(2,27),(1,28),(0,28),(0,28),(0,28),(0,28),(0,28),(0,28),(0,27),(1,26))),
 SpeechFrame(0x28,0x900f9,344,'d8eb436c71d8abd095734aa53b18571a0ffb55424387aa6fbaf99440db3439de','OCTO!',13,((5,28),(4,29),(3,30),(3,31),(3,31),(3,31),(3,31),(3,31),(3,31),(4,31),(5,30))),
 SpeechFrame(0x30,0x909b7,310,'6b88aab19439ac0ed7e79099e04f5b85858994687eeb9f1df510d4b4751fb1d4','YOU',15,((3,27),(2,28),(1,29),(1,29),(1,29),(1,29),(1,29),(1,29),(1,28),(1,27))),
 SpeechFrame(0x31,0x90aed,348,'8d2029821ecd3956ed5cc2dfd067d1bfa815eee3dcfccca9528c492f425295a6','PERV!',13,((2,29),(1,30),(0,31),(0,31),(0,31),(0,31),(0,31),(0,31),(0,30),(0,29),(0,28))),
 SpeechFrame(0x38,0x9128b,324,'044ba90d39fd07b1d7ff28f5be3fbcccdcd896593aa4e8d6652724258514402d','HEHE',15,((2,29),(1,30),(0,31),(0,31),(0,31),(0,31),(0,31),(0,31),(1,31),(2,31),(3,30))),
 SpeechFrame(0x39,0x913cf,319,'b107904bc68beee796f32bc721dbc13bd1b640a591c357b0b57bc666aa023666','SPUD!',15,((2,29),(1,30),(0,31),(0,31),(0,31),(0,31),(0,31),(0,31),(0,31),(1,31),(2,31))),
)
# Every glyph is authored on the native 3x5 grid. One blank column separates it.
FONT={
 'S':('111','100','111','001','111'),'T':('111','010','010','010','010'),
 'U':('101','101','101','101','111'),'P':('110','101','110','100','100'),
 'I':('111','010','010','010','111'),'D':('110','101','101','101','110'),
 '!':('010','010','010','000','010'),'N':('101','111','111','111','101'),
 'Y':('101','101','010','010','010'),'A':('010','101','111','101','101'),
 'H':('101','101','111','101','101'),'O':('111','101','101','101','111'),
 'C':('111','100','100','100','111'),'V':('101','101','101','101','010'),
 'E':('111','100','110','100','111'),'R':('110','101','110','101','101'),
}

def decode(data:bytes,address:int=0)->tuple[bytes,int]:
    start=address
    if address<0 or address+3>len(data):raise ValueError('Truncated DB1 header')
    length=int.from_bytes(data[address:address+2],'big');address+=3
    if length!=512:raise ValueError('Speech stream must declare 512 bytes')
    out=bytearray()
    while len(out)<length:
        if address>=len(data):raise ValueError('Truncated DB1 token')
        token=data[address];address+=1;count=(token&127)+1
        if len(out)+count>length:raise ValueError('DB1 token exceeds output')
        if token&128:
            if address>=len(data):raise ValueError('Truncated DB1 distance')
            distance=data[address]+1;address+=1
            if distance>len(out):raise ValueError('Invalid DB1 backreference')
            for _ in range(count):out.append(out[-distance])
        else:
            if address+count>len(data):raise ValueError('Truncated DB1 literal')
            out.extend(data[address:address+count]);address+=count
    return bytes(out),address-start

def encode(data:bytes,reserved:int=0)->bytes:
    if len(data)!=512 or not 0<=reserved<=255:raise ValueError('Expected one 512-byte frame')
    n=len(data);cost=[10**9]*(n+1);choice=[None]*n;cost[n]=0
    for p in range(n-1,-1,-1):
        for count in range(1,min(128,n-p)+1):
            candidate=1+count+cost[p+count]
            if candidate<cost[p]:cost[p]=candidate;choice[p]=(count,0)
        longest=0;best_distance=0
        for distance in range(1,min(256,p)+1):
            count=0
            while count<min(128,n-p) and data[p+count]==data[p+count-distance]:count+=1
            if count>longest:longest=count;best_distance=distance
        for count in range(2,longest+1):
            candidate=2+cost[p+count]
            if candidate<cost[p]:cost[p]=candidate;choice[p]=(count,best_distance)
    out=bytearray(b'\x02\x00'+bytes([reserved]));p=0
    while p<n:
        count,distance=choice[p]
        if distance:out.extend((128+count-1,distance-1))
        else:out.append(count-1);out.extend(data[p:p+count])
        p+=count
    return bytes(out)

def _offset(x:int,y:int)->tuple[int,int]:
    if not 0<=x<32 or not 0<=y<32:raise ValueError('Outside native frame')
    return ((x//8)*4+y//8)*32+(y%8)*4+(x%8)//2,0 if x&1 else 4

def pixel(data:bytes,x:int,y:int)->int:
    if len(data)!=512:raise ValueError('Expected 512-byte frame')
    pos,shift=_offset(x,y);return (data[pos]>>shift)&15

def _set(data:bytearray,x:int,y:int,value:int):
    pos,shift=_offset(x,y);data[pos]=(data[pos]&~(15<<shift))|(value<<shift)

def interior(frame:SpeechFrame)->frozenset:
    # Original contour edges are excluded. Frame30's hand starts on row11:
    # its explicit mask stops at row10, before any actor pixel can change.
    return frozenset((x,y) for y,(left,right) in enumerate(frame.bounds,1) for x in range(left+1,right))

def replace_letters(original:bytes,frame:SpeechFrame)->bytes:
    if len(original)!=512:raise ValueError('Expected 512-byte frame')
    if not frame.text or any(c not in FONT for c in frame.text):raise ValueError('Unsupported speech glyph')
    mask=interior(frame);out=bytearray(original)
    for x,y in mask:_set(out,x,y,1)
    width=4*len(frame.text)-1
    safe_left=max(left+1 for left,right in frame.bounds[3:8]);safe_right=min(right for left,right in frame.bounds[3:8])
    left=safe_left+(safe_right-safe_left-width)//2
    for i,char in enumerate(frame.text):
        for dy,row in enumerate(FONT[char]):
            for dx,bit in enumerate(row):
                x,y=left+i*4+dx,4+dy
                if (x,y) not in mask:raise ValueError('Speech does not fit interior')
                if bit=='1':_set(out,x,y,frame.ink)
    result=bytes(out)
    for y in range(32):
        for x in range(32):
            if (x,y) not in mask and pixel(original,x,y)!=pixel(result,x,y):raise ValueError('Outside-interior change')
    return result

def translate(source:bytes)->tuple[bytes,dict]:
    sha=hashlib.sha256(source).hexdigest()
    if sha!=SOURCE_SHA256:raise ValueError('Expected qualified English source SHA256')
    output=bytearray(source);frames=[]
    for frame in FRAMES:
        stream=source[frame.address:frame.address+frame.capacity]
        if hashlib.sha256(stream).hexdigest()!=frame.guard:raise ValueError('Speech stream guard mismatch')
        pointer=0x8d942+int.from_bytes(source[0x8d942+frame.frame*2:0x8d944+frame.frame*2],'big')
        if pointer!=frame.address:raise ValueError('Speech pointer guard mismatch')
        before,used=decode(stream)
        if used!=frame.capacity:raise ValueError('Speech capacity guard mismatch')
        after=replace_letters(before,frame);packed=encode(after,stream[2])
        if len(packed)>frame.capacity:raise ValueError(f'Frame {frame.frame:02X} exceeds original capacity')
        if decode(packed)!=(after,len(packed)):raise ValueError('Speech codec roundtrip failed')
        output[frame.address:frame.address+len(packed)]=packed
        frames.append(dict(frame=f'{frame.frame:02X}',text=frame.text,address=hex(frame.address),capacity=frame.capacity,encoded_bytes=len(packed),outside_interior_exact=True))
    checksum=sum(int.from_bytes(output[i:i+2],'big') for i in range(512,len(output),2))&65535
    output[0x18e:0x190]=checksum.to_bytes(2,'big');result=bytes(output)
    return result,dict(source_sha256=sha,output_sha256=hashlib.sha256(result).hexdigest(),frames=frames,
        scope='Draft P-chan graphics only; preserve all eight frames, actor pixels, native palette, pointers and durations',
        wording='STU/PID!, NYAH/OCTO!, YOU/PERV!, HEHE/SPUD!; final small kana in baka and idiomatic taunt sense remain uncertain; SPUD preserves literal potato rather than inventing hick')

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('source',type=Path);parser.add_argument('--output-dir',type=Path,required=True);args=parser.parse_args()
    local=(Path(__file__).resolve().parents[1]/'local').resolve();destination=args.output_dir.resolve()
    if destination==local or not destination.is_relative_to(local):parser.error('Use a fresh child directory under project local/')
    if destination.exists():parser.error('Output directory already exists')
    source=args.source.read_bytes();result,report=translate(source)
    destination.mkdir(parents=True);target=destination/'db1-en-speech-draft.md'
    with target.open('xb') as f:f.write(result)
    (destination/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    # Optional private review sheet; palette-index grayscale, never copied to game.
    from PIL import Image,ImageDraw
    sheet=Image.new('RGB',(640,8*144),(40,60,80));draw=ImageDraw.Draw(sheet)
    for row,frame in enumerate(FRAMES):
        for column,data in enumerate((source,result)):
            raw,_=decode(data,frame.address);image=Image.new('RGB',(32,32));image.putdata([(pixel(raw,x,y)*17,)*3 for y in range(32) for x in range(32)])
            sheet.paste(image.resize((128,128),Image.Resampling.NEAREST),(80+column*280,row*144))
        draw.text((4,row*144+4),f'{frame.frame:02X}',fill='white');draw.text((360,row*144+130),frame.text,fill='white')
    sheet.save(destination/'speech-before-after.png');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
