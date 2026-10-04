import struct,zlib,sys,os
class Tank:
    def __init__(s,path):
        s.f=open(path,'rb'); h=s.f.read(40)
        s.ver,s.dirset,s.fileset,s.indexsize,s.dataoff=struct.unpack('<5I',h[8:28])
        s.f.seek(0,2); end=s.f.tell()
        s.f.seek(s.dirset); s.idx=s.f.read(end-s.dirset)
        s.dirs={}; s.files={}
        s._parse()
    def _nstr(s,o):
        n=struct.unpack_from('<H',s.idx,o)[0]
        name=s.idx[o+2:o+2+n].decode('latin1')
        tot=(2+n+1+3)&~3
        return name,o+tot
    def _parse(s):
        d=s.idx
        nd=struct.unpack_from('<I',d,0)[0]
        doffs=struct.unpack_from('<%dI'%nd,d,4)
        for o in doffs:
            parent,cnt=struct.unpack_from('<II',d,o)
            name,_=s._nstr(o+16)
            s.dirs[o]=(parent,name)
        fb=s.fileset-s.dirset
        nf=struct.unpack_from('<I',d,fb)[0]
        foffs=struct.unpack_from('<%dI'%nf,d,fb+4)
        for o in foffs:
            p=fb+o
            parent,size,off,crc=struct.unpack_from('<4I',d,p)
            fmt,flags=struct.unpack_from('<HH',d,p+24)
            name,q=s._nstr(p+28)
            chunks=None
            if fmt!=0:
                csize,chunksize=struct.unpack_from('<II',d,q); q+=8
                nch=(size+chunksize-1)//chunksize if chunksize else 0
                chunks=[struct.unpack_from('<4I',d,q+16*i) for i in range(nch)]
            s.files[s.path(parent)+'/'+name]=(size,off,fmt,chunks)
    def path(s,o):
        parts=[]
        while o in s.dirs and s.dirs[o][1]:
            parts.append(s.dirs[o][1]); o=s.dirs[o][0]
            if len(parts)>40: break
        return '/'.join(reversed(parts))
    def read(s,name):
        size,off,fmt,chunks=s.files[name]
        if fmt==0:
            s.f.seek(s.dataoff+off); return s.f.read(size)
        out=b''
        for usz,csz,extra,coff in chunks:
            s.f.seek(s.dataoff+off+coff); raw=s.f.read(csz+extra)
            if usz==csz: out+=raw[:usz]
            else:
                try: out+=zlib.decompress(raw[:csz])+raw[csz:csz+extra]
                except Exception as e: out+=b'<<ERR %s fmt %d>>'%(str(e).encode(),fmt)
        return out
if __name__=='__main__':
    t=Tank(sys.argv[1])
    pat=sys.argv[2] if len(sys.argv)>2 else ''
    for n in sorted(t.files):
        if pat in n.lower(): print(n,t.files[n][0],t.files[n][2])
