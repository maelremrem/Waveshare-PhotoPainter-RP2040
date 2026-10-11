from pathlib import Path
import tempfile, subprocess
source=(Path(__file__).resolve().parents[1] / 'src/Gallery.cpp').read_text()
start=source.index('int32_t jpegRead(')
end=source.index('const char *jpegErrorText', start)
snippet=source[start:end]
cpp=r'''
#include <cassert>
#include <vector>
#include <stdint.h>
#include <stddef.h>
struct File {
    std::vector<uint8_t> bytes;
    uint32_t p = 0;
    File(size_t n) { for (size_t i=0;i<n;++i) bytes.push_back(uint8_t(i%256)); }
    uint32_t position() const { return p; }
    bool seek(uint32_t x) { if (x>bytes.size()) return false; p=x; return true; }
    size_t read(uint8_t *out, size_t n) { size_t avail=bytes.size()-p; if (n>avail) n=avail; for(size_t i=0;i<n;++i)out[i]=bytes[p+i];p+=n;return n; }
};
struct JPEGFILE { int32_t iPos=0, iSize=0; void *fHandle=nullptr; };
''' + snippet + r'''
int main() {
    File f(1000); JPEGFILE h; h.iSize=1000; h.fHandle=&f;
    uint8_t b[500]{};
    assert(jpegRead(&h,b,100)==100); assert(h.iPos==100 && f.position()==100 && b[0]==0 && b[99]==99);
    assert(jpegSeek(&h,900)==900 && h.iPos==900 && f.position()==900);
    assert(jpegRead(&h,b,500)==100 && h.iPos==1000 && f.position()==1000);
    assert(jpegRead(&h,b,500)==0 && h.iPos==1000);
    assert(jpegSeek(&h,-1)==-1 && h.iPos==1000);
    assert(jpegSeek(&h,1001)==-1 && h.iPos==1000);
    assert(jpegSeek(&h,0)==0 && h.iPos==0);
    assert(jpegRead(&h,b,500)==500 && h.iPos==500);
    assert(f.seek(222));
    assert(jpegRead(&h,b,7)==7 && h.iPos==507 && f.position()==507 && b[0]==uint8_t(500%256));
}
'''
with tempfile.TemporaryDirectory() as t:
    p=Path(t)/'test.cpp';p.write_text(cpp)
    r=subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(p),'-o',t+'/check'],capture_output=True,text=True)
    if r.returncode: print(r.stderr); raise SystemExit(r.returncode)
    r=subprocess.run([t+'/check'],capture_output=True,text=True)
    if r.returncode: print(r.stderr); raise SystemExit(r.returncode)
print('JPEG stream callbacks: PASS (read/seek/EOF/resync/invalid seek)')
