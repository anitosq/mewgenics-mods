"""Generate original vector SWF controls for the native inventory experiment.

References the installed game's Edmundm font, just like house.swf.
No game font outlines, game artwork or third-party mod assets are bundled.
"""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]

class Bits:
    def __init__(self): self.bits = []
    def put(self, value, count):
        self.bits.extend((value >> i) & 1 for i in reversed(range(count)))
    def bytes(self):
        self.bits.extend([0] * (-len(self.bits) % 8))
        return bytes(sum(self.bits[i+j] << (7-j) for j in range(8)) for i in range(0, len(self.bits), 8))

def width(*values): return max(2, max(abs(v).bit_length()+1 for v in values))
def rect(x0,y0,x1,y1):
    b=Bits(); n=width(x0,y0,x1,y1); b.put(n,5)
    for v in (x0,x1,y0,y1): b.put(v,n)
    return b.bytes()
def tag(code, body=b''):
    n=len(body)
    return struct.pack('<H', code*64+min(n,63))+(struct.pack('<I',n) if n>=63 else b'')+body
def matrix(): return b'\0'  # no scale, no skew, zero translation bits

class Movie:
    def __init__(self):
        # Same empty ImportAssets2 dependency used by the game's house/ui SWFs.
        self.tags=[tag(71,b'fonts.swf\0\1\0\0\0')];self.next=1;self.exports=[]
    def shape(self, draw, color):
        ident=self.next;self.next+=1;b=Bits();b.put(1,4);b.put(0,4)
        draw(b);b.put(0,6)
        # Generous local bounds include the full toolbar or popup.
        body=struct.pack('<H',ident)+rect(-20,-400,12020,12000)+b'\1\0'+bytes(color)+b'\0'+b.bytes()
        self.tags.append(tag(32,body));return ident
    def polygon(self,points,color):
        def draw(b):
            coords=[(round(x*20),round(y*20)) for x,y in points]
            x,y=coords[0];b.put(0,1);b.put(3,5);n=width(x,y);b.put(n,5);b.put(x,n);b.put(y,n);b.put(1,1)
            for nx,ny in coords[1:]+coords[:1]:
                dx,dy=nx-x,ny-y
                if dx or dy:
                    n=width(dx,dy);assert n<=17
                    b.put(3,2);b.put(n-2,4);b.put(1,1);b.put(dx,n);b.put(dy,n)
                x,y=nx,ny
        return self.shape(draw,color)
    def text(self,text,x,y,size=22,color=(54,54,54,255)):
        return self.field('Edmundm',x,y-size,550,size+9,size,text,color=color)
    def sprite(self,name,parts,export=True):
        ident=self.next;self.next+=1;body=struct.pack('<HH',ident,1)
        for depth,part in enumerate(parts,1):
            if isinstance(part,tuple):
                ident_part,instance=part
                body+=tag(26,b'\x26'+struct.pack('<HH',depth,ident_part)+matrix()+instance.encode()+b'\0')
            else:body+=tag(26,b'\6'+struct.pack('<HH',depth,part)+matrix())
        body+=tag(1)+tag(0);self.tags.append(tag(39,body))
        if export:self.exports.append((ident,name))
        return ident
    def feedback(self,parts,rectangles):
        # Children share their parent's coordinates and transform. Never stretch
        # a generic overlay: that also stretches its inset and underline.
        for index,(x,y,w,h) in enumerate(rectangles):
            for pressed in range(2):
                name=f'fx{pressed}_{index}'
                shapes=[self.polygon([(x+3,y+3),(x+w-4,y+3),(x+w-3,y+h-4),(x+3,y+h-4)],(54,50,42,45 if pressed else 17)),
                        self.polygon([(x+4,y+h-5),(x+w-4,y+h-5),(x+w-4,y+h-3),(x+4,y+h-3)],(54,50,42,170 if pressed else 105))]
                parts.append((self.sprite(name,shapes,export=False),name))
        return parts
    def paper(self,x,y,w,h,selected=False):
        # Original irregular outlines; native house palette, without replacing house.swf.
        edge=[(x+1,y+1),(x+w*.42,y),(x+w-2,y+1),(x+w,y+h-3),(x+w-2,y+h),(x+w*.36,y+h-1),(x,y+h-2)]
        shadow=self.polygon([(a+1,b+2) for a,b in edge],(38,33,24,55))
        outer=self.polygon(edge,(54,50,42,255))
        inner=self.polygon([(x+3,y+3),(x+w*.42,y+2),(x+w-4,y+3),(x+w-2,y+h-4),(x+w-4,y+h-2),(x+2,y+h-4)],(190,204,164,255) if selected else (232,225,206,255))
        return [shadow,outer,inner]
    def button(self,label,x,y,w,selected=False,center=True,size=20):
        parts=self.paper(x,y,w,36,selected)
        if label:parts.append(self.field('Edmundm',x+4,y+3,w-8,30,size,label,align=2 if center else 0))
        return parts
    def dropdown(self,label,x,w,selected=False):
        parts=self.button('',x,0,w,selected)
        if label:parts.append(self.field('Edmundm',x+9,3,w-34,30,20,label))
        # A hand-drawn chevron, kept separate from the field's text width.
        parts.append(self.polygon([(x+w-22,14),(x+w-16,19),(x+w-11,13),(x+w-9,15),(x+w-16,23),(x+w-24,16)],(54,50,42,255)))
        return parts
    def field(self,fontclass,x,y,w,h,size=20,initial='',align=0,color=(54,54,54,255)):
        ident=self.next;self.next+=1
        # HasFontClass, not HasFont: resolve the installed game's font and fallbacks.
        flags=0x8000|0x0800|0x0400|0x0080|0x0020|0x0010|0x0001
        body=struct.pack('<H',ident)+rect(x*20,y*20,(x+w)*20,(y+h)*20)
        body+=struct.pack('>H',flags)+fontclass.encode()+b'\0'+struct.pack('<H',size*20)+bytes(color)
        body+=struct.pack('<BHHHh',align,0,0,0,0)+b'\0'+initial.encode()+b'\0'
        self.tags.append(tag(37,body));return ident
    def write(self,path):
        exports=struct.pack('<H',len(self.exports))+b''.join(struct.pack('<H',i)+n.encode()+b'\0' for i,n in self.exports)
        body=rect(0,0,12800,7200)+struct.pack('<HH',24*256,1)+tag(69,struct.pack('<I',8))+b''.join(self.tags)+tag(76,exports)+tag(1)+tag(0)
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'FWS\x0a'+struct.pack('<I',len(body)+8)+body)

m=Movie(); names=['Any rarity','Common','Uncommon','Rare','Very rare']
types=['All','Weapon consumables','Item consumables','Worn','Broken','Reusable weapons']
dynamic_font='Edmundm'
for category,category_label in enumerate(types):
    for rarity,label in enumerate(names):
        parts=[]
        for text,x,w,on in [(category_label,0,230,category!=0),(label,238,135,rarity!=0),('',381,114,False)]:
            parts+=m.dropdown(text,x,w,on)
        parts+=m.button('Reset',503,0,68)
        parts.append((m.field(dynamic_font,390,3,80,30,20,'All sets'),'sets'))
        m.sprite(f'IQBar{category}{rarity}',m.feedback(parts,[(0,0,230,36),(238,0,135,36),(381,0,114,36),(503,0,68,36)]))
for active in range(len(names)):
    # Compensate for the compact popup's 70% scale: a 36-unit button on screen.
    parts=m.paper(0,0,330,446)+[m.text('Rarity',17,41,31)]
    parts+=m.paper(10,61,310,36/0.70,active==0)
    parts.append(m.field(dynamic_font,14,67,302,40,28,'Any',align=2))
    for i,label in enumerate(names[1:]):
        x,y=10+(i%2)*160,126+(i//2)*160
        parts+=m.paper(x,y,150,150,i+1==active)
    m.sprite(f'IQRarity{active}',m.feedback(parts,[(10,61,310,36/0.70)]+[(10+(i%2)*160,126+(i//2)*160,150,150) for i in range(4)]))
# Text is rendered above the native rarity artwork, centered on each tile.
parts=[]
for i,label in enumerate(names[1:]):
    x,y=10+(i%2)*160,126+(i//2)*160
    parts.append(m.field(dynamic_font,x+4,y+60,142,30,20,label,align=2))
m.sprite('IQRarityLabels',parts)
for active in range(len(types)):
    parts=m.paper(0,0,280,311)+[m.text('Item type',12,29,22)]
    for i,label in enumerate(types):parts+=m.button(label,10,43+i*43,260,i==active)
    m.sprite(f'IQType{active}',m.feedback(parts,[(10,43+i*43,260,36) for i in range(len(types))]))
for direction in range(2):
    parts=m.button('',0,0,32)
    points=[(7,24),(16,11),(25,24)] if direction==0 else [(7,12),(25,12),(16,25)]
    parts.append(m.polygon(points,(39,37,32,255)))
    m.sprite('IQUp' if direction==0 else 'IQDown',m.feedback(parts,[(0,0,32,36)]))
for focused in range(2):
    parts=m.button('',0,0,571,focused)
    parts+=[(m.field(dynamic_font,10,3,505,31,20,'Search name, description, set...'),'query')]
    parts+=m.button('x',533,0,38)
    m.sprite(f'IQSearch{focused}',m.feedback(parts,[(533,0,38,36)]))
# Additional inset keeps both counts comfortably inside the lighter panel.
m.sprite('IQCount',[(m.field(dynamic_font,18,0,562,24,18,'Matches'),'count')])
for active in range(4):
    parts=m.paper(0,0,360,479)+[m.text('Sets (match any selected)',12,28,22)]
    for i,label in enumerate(['All','Any','No set']):parts+=m.button(label,10+i*115,38,110,active==i,center=True)
    parts+=m.button('',10,82,340)
    parts.append((m.field(dynamic_font,18,84,291,31,20,'Find a set...'),'query'))
    parts+=m.button('x',316,82,34)
    for i in range(9):
        parts.append((m.field(dynamic_font,12,126+i*33,338,31,20,''),f'row{i}'))
    parts+=m.button('Up',10,429,62)+m.button('Down',80,429,77)+m.button('Clear',166,429,78)+m.button('Done',253,429,97)
    rectangles=[(10+i*115,38,110,36) for i in range(3)]+[(316,82,34,36),(10,82,306,36)]
    rectangles += [(10,126+i*33,340,31) for i in range(9)]
    rectangles += [(10,429,62,36),(80,429,77,36),(166,429,78,36),(253,429,97,36)]
    m.feedback(parts,rectangles)
    for i,(label,x,width_) in enumerate([('Up',10,62),('Down',80,77)]):
        disabled=m.paper(x,429,width_,36)
        disabled.append(m.field(dynamic_font,x+4,432,width_-8,30,20,label,align=2,color=(136,130,117,255)))
        parts.append((m.sprite('disabled'+str(i),disabled,export=False),'disabled'+str(i)))
    m.sprite(f'IQSets{active}',parts)
m.write(ROOT/'work/native-assets/swfs/improved_inventory.swf')
(ROOT/'work/native-assets/swfs/swflist.gon.append').write_text('game [ improved_inventory.swf ]\n')
print(f'Generated {len(m.exports)} original native toolbar/popup/arrow symbols.')
