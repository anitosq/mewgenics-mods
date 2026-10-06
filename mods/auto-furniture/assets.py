"""Paper controls matching Improved Inventory; native font and stat art at runtime.

SWF encoding adapted from Improved Inventory, MIT, copyright 2026 anitosq.
"""
from pathlib import Path
import struct
import localization

PANEL_HITS = [(558,0,42,48)] + [(18,90+i*44,215,36) for i in range(5)]
PANEL_HITS += [(x,90+i*44,76,36) for i in range(5) for x in (238,320)]
PANEL_HITS += [(18,324,420,28),(450,320,132,38)]
PANEL_HITS += [(18+i*144,370,132,38) for i in range(4)]
PANEL_HITS += [(18,546,276,38),(306,546,276,38)]
PIN_HITS = [(558,0,42,48),(18,436,55,36),(85,436,55,36),(450,436,132,36)]
# Native catparts.swf Rare/Quest item tints, sampled at gray 177 and blended 35% over paper.
CALCULATE_FILL = (224,219,177,255)
APPLY_FILL = (187,212,170,255)


class Bits:
    def __init__(self): self.bits = []
    def put(self, value, count):
        self.bits.extend((value >> i) & 1 for i in reversed(range(count)))
    def bytes(self):
        self.bits.extend([0] * (-len(self.bits) % 8))
        return bytes(sum(self.bits[i+j] << (7-j) for j in range(8)) for i in range(0, len(self.bits), 8))


def width(*values): return max(2, max(abs(v).bit_length()+1 for v in values))
def rect(x0, y0, x1, y1):
    b = Bits(); n = width(x0, y0, x1, y1); b.put(n, 5)
    for v in (x0, x1, y0, y1): b.put(v, n)
    return b.bytes()


def tag(code, body=b''):
    n = len(body)
    return struct.pack('<H', code*64+min(n, 63))+(struct.pack('<I', n) if n >= 63 else b'')+body


class Movie:
    def __init__(self):
        self.tags = [tag(71, b'fonts.swf\0\1\0\0\0')]; self.next = 1; self.exports = []
    def polygon(self, points, color):
        ident = self.next; self.next += 1
        b = Bits(); b.put(1, 4); b.put(0, 4)
        coords = [(round(x*20), round(y*20)) for x, y in points]
        x, y = coords[0]; b.put(0, 1); b.put(3, 5); n = width(x, y)
        b.put(n, 5); b.put(x, n); b.put(y, n); b.put(1, 1)
        for nx, ny in coords[1:]+coords[:1]:
            dx, dy = nx-x, ny-y
            if dx or dy:
                n = width(dx, dy); assert n <= 17
                b.put(3, 2); b.put(n-2, 4); b.put(1, 1); b.put(dx, n); b.put(dy, n)
            x, y = nx, ny
        b.put(0, 6)
        self.tags.append(tag(32, struct.pack('<H', ident)+rect(min(p[0] for p in coords),min(p[1] for p in coords),max(p[0] for p in coords),max(p[1] for p in coords))+b'\1\0'+bytes(color)+b'\0'+b.bytes()))
        return ident
    def box(self, x, y, w, h, color):
        return self.polygon([(x,y),(x+w,y),(x+w,y+h),(x,y+h)], color)
    def paper(self, x, y, w, h, fill=(232,225,206,255)):
        edge=[(x+1,y+1),(x+w*.42,y),(x+w-2,y+1),(x+w,y+h-3),
              (x+w-2,y+h),(x+w*.36,y+h-1),(x,y+h-2)]
        return [self.polygon([(a+1,b+2) for a,b in edge],(38,33,24,55)),
                self.polygon(edge,(54,50,42,255)),
                self.polygon([(x+3,y+3),(x+w*.42,y+2),(x+w-4,y+3),
                              (x+w-2,y+h-4),(x+w-4,y+h-2),(x+2,y+h-4)],fill)]
    def field(self, x, y, w, h, size, initial='', align=0, wrap=False):
        ident = self.next; self.next += 1
        flags = 0x8000|0x0800|0x0400|0x0080|0x0020|0x0010|0x0001
        if wrap: flags |= 0x4000|0x2000
        body = struct.pack('<H',ident)+rect(x*20,y*20,(x+w)*20,(y+h)*20)
        body += struct.pack('>H',flags)+b'Edmundm\0'+struct.pack('<H',size*20)+bytes((54,50,42,255))
        body += struct.pack('<BHHHh',align,0,0,0,0)+b'\0'+initial.encode()+b'\0'
        self.tags.append(tag(37,body)); return ident
    def sprite(self, name, parts, export=True):
        ident = self.next; self.next += 1; body = struct.pack('<HH',ident,1)
        for depth, part in enumerate(parts,1):
            if isinstance(part,tuple):
                value, instance = part
                body += tag(26,b'\x26'+struct.pack('<HH',depth,value)+b'\0'+instance.encode()+b'\0')
            else: body += tag(26,b'\6'+struct.pack('<HH',depth,part)+b'\0')
        self.tags.append(tag(39,body+tag(1)+tag(0)))
        if export: self.exports.append((ident,name))
        return ident
    def feedback(self, parts, rectangles):
        for i,(x,y,w,h) in enumerate(rectangles):
            for state in range(3):
                name=f'fx{state}_{i}'
                color=(232,225,206,175) if state==2 else (54,50,42,45 if state else 17)
                shapes=[self.box(x+2,y+2,w-4,h-4,color)]
                if state<2: shapes.append(self.box(x+4,y+h-5,w-8,2,(54,50,42,170 if state else 105)))
                parts.append((self.sprite(name,shapes,export=False),name))
        return parts
    def write(self, path):
        exports = struct.pack('<H',len(self.exports))+b''.join(struct.pack('<H',i)+n.encode()+b'\0' for i,n in self.exports)
        body = rect(0,0,12800,7200)+struct.pack('<HH',24*256,1)+tag(69,struct.pack('<I',8))+b''.join(self.tags)+tag(76,exports)+tag(1)+tag(0)
        path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(b'FWS\x0a'+struct.pack('<I',len(body)+8)+body)


def build():
    m = Movie(); ink = (54,50,42,255)
    m.sprite('AFShade',[m.box(-1600,-1000,3200,2000,(0,0,0,80))])
    parts = m.paper(0,0,32,32)
    def wand(points,color):
        return m.polygon([(16+(x-16)*0.82,16+(y-16)*0.82) for x,y in points],color)
    parts += [wand([(5,26),(10,20),(14,17),(19,11),(22,13),(18,19),(13,23),(9,28),(7,29)],ink),
              wand([(8,26),(12,21),(16,18),(20,13),(19,17),(15,21),(9,27)],(168,156,131,255)),
              wand([(19,3),(21,3),(23,8),(28,8),(29,10),(25,14),(26,19),(24,20),
                         (20,17),(15,20),(13,19),(15,13),(11,10),(12,8),(18,8)],ink),
              wand([(20,6),(22,11),(26,10),(22,13),(23,17),(20,14),(16,17),(17,12),(14,10),(19,10)],(232,225,206,255))]
    m.sprite('AFRoom',m.feedback(parts,[(0,0,32,32)]))
    parts = m.paper(0,0,600,604)
    parts += [(m.field(18,10,530,36,27,'Auto Furniture'),'title'),m.field(565,10,30,36,26,'X'),
              (m.field(18,56,210,30,19,'Stat'),'stat'),(m.field(238,56,76,30,19,'Min'),'min'),
              (m.field(320,56,76,30,19,'Max'),'max'),(m.field(410,56,75,30,19,'Before',align=2),'before'),
              (m.field(493,56,90,30,17,'Best found',align=2),'best')]
    parts += [m.box(18,82,564,1,(54,50,42,90)),m.box(18,313,564,1,(54,50,42,90))]
    for i,label in enumerate(('Comfort','Stimulation','Health','Mutation','Appeal')):
        y=90+i*44
        parts += m.paper(18,y+3,28,28)+[(m.field(21,y+2,25,30,23),'check'+str(i)),(m.field(92,y+3,141,32,20,label),'stat'+str(i))]
        for prefix,x in [('min',238),('max',320)]:
            parts += m.paper(x,y,76,36)+[(m.field(x+3,y+4,70,30,18,align=2),prefix+str(i))]
        parts += [(m.field(410,y+4,75,30,21,align=2),'old'+str(i)),(m.field(493,y+4,90,30,21,align=2),'new'+str(i))]
    parts += m.paper(18,324,28,28)+[(m.field(21,322,25,30,23),'utility'),(m.field(58,321,380,34,21,'Include utility furniture'),'utility_label')]
    parts += m.paper(450,320,132,38)+[(m.field(454,325,124,30,22,'Clear',align=2),'clear')]
    for label,x,fill in [('Calculate',18,CALCULATE_FILL),('Apply',162,APPLY_FILL),
                         ('Undo',306,(232,225,206,255)),('Pins',450,(232,225,206,255))]:
        field = m.field(x+4,375,124,30,22,label,align=2)
        parts += m.paper(x,370,132,38,fill=fill)+[(field,label.lower())]
    parts += [(m.field(18,426,564,44,17,wrap=True),'status'),(m.field(18,476,72,28,18),'change')]
    for i in range(5):
        parts += [(m.field(118+i*94,476,72,28,18,align=0),'delta'+str(i))]
    parts += [m.box(18,522,564,1,(54,50,42,90))]
    for label,x,name in [('Return room',18,'return_room'),('Return all rooms',306,'return_all')]:
        parts += m.paper(x,546,276,38)+[(m.field(x+4,551,268,30,22,label,align=2),name)]
    m.sprite('AFPanel',m.feedback(parts,PANEL_HITS))
    parts=m.paper(0,0,600,490)+[(m.field(18,10,530,36,27,'Pinned furniture'),'title'),m.field(565,10,30,36,26,'X')]
    for label,x,w in [('',18,55),('',85,55),('Done',450,132)]:
        field=m.field(x+4,439,w-8,31,23,label,align=2)
        parts+=m.paper(x,436,w,36)+[(field,'done') if label else field]
    parts += [m.polygon([(51,445),(40,454),(51,463),(47,466),(32,454),(47,442)],ink),
              m.polygon([(107,445),(118,454),(107,463),(111,466),(126,454),(111,442)],ink)]
    parts+=[(m.field(160,440,280,30,20),'page')]
    parts += [(m.field(18,200,564,36,22,align=2),'empty')]
    m.sprite('AFPins',m.feedback(parts,PIN_HITS))
    m.sprite('AFPinRow',m.feedback(m.paper(18,2,28,28)+[(m.field(21,0,25,30,23),'check'),
                                         (m.field(58,0,520,32,20),'item')],[(18,0,564,32)]))
    m.sprite('AFHint',m.paper(0,0,160,28)+[(m.field(4,4,152,22,16,align=2),'hint')])
    root=Path(__file__).resolve().parent/'build/data-mod/swfs'
    m.write(root/'auto_furniture.swf')
    (root/'swflist.gon.append').write_text('game [ auto_furniture.swf ]\n',encoding='ascii')
    localization.build(root.parents[1])
    # Rendering, hover, and input share the same bounds.
    layout='typedef struct {double x,y,w,h;} UIBox;\n'
    for name,boxes in [('panel_boxes',PANEL_HITS),('pin_boxes',PIN_HITS)]:
        layout+='static const UIBox '+name+'[]={'+','.join('{'+','.join(map(str,b))+'}' for b in boxes)+'};\n'
    (root.parents[1]/'ui-layout.h').write_text(layout,encoding='ascii')


if __name__ == '__main__': build()
