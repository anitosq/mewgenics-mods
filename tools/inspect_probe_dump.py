"""Print exception and module-relative stack candidates from a local minidump.

Stack matches are return-address candidates, not an unwound backtrace. No memory
strings, user data, or full dump payloads are printed.
"""
import argparse
from pathlib import Path
import struct


def inspect(path):
    data=path.read_bytes()
    def u32(at): return struct.unpack_from('<I',data,at)[0]
    def u64(at): return struct.unpack_from('<Q',data,at)[0]
    if data[:4]!=b'MDMP': raise ValueError('Not a minidump')
    streams={u32(at):(u32(at+4),u32(at+8)) for at in range(u32(12),u32(12)+12*u32(8),12)}
    mods=[]
    loc=streams[4][1]
    for at in range(loc+4,loc+4+108*u32(loc),108):
        name_at=u32(at+20)
        name=data[name_at+4:name_at+4+u32(name_at)].decode('utf-16-le')
        mods.append((u64(at),u32(at+8),Path(name).name))
    def owner(address):
        return next((f'{name}+0x{address-base:x}' for base,size,name in mods if base<=address<base+size),None)
    loc=streams[6][1]
    print('Exception:',hex(u32(loc+8)), 'at',owner(u64(loc+24)))
    print('Parameters:',[hex(u64(loc+40+8*i)) for i in range(min(15,u32(loc+32)))])
    thread=u32(loc)
    context=u32(loc+164)
    stack_pointer=u64(context+152)
    print('RIP:',owner(u64(context+248)))
    loc=streams[3][1]
    for at in range(loc+4,loc+4+48*u32(loc),48):
        if u32(at)!=thread: continue
        base,size,offset=u64(at+24),u32(at+32),u32(at+36)
        start=max(0,stack_pointer-base)
        for relative in range(start,min(start+768,size-7),8):
            found=owner(u64(offset+relative))
            if found: print(f'  stack+{relative-start:03x}: {found}')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dump',type=Path)
    inspect(parser.parse_args().dump)
