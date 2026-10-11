from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / 'src/Gallery.cpp').read_text()
probe = source[source.index('uint8_t jpegSofMarker('):source.index('int jpegDraw(', source.index('uint8_t jpegSofMarker('))]
input_func = source[source.index('size_t tinyJpegRead('):source.index('int tinyJpegDraw(', source.index('size_t tinyJpegRead('))]

code = r'''
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cassert>
struct File {
    std::vector<uint8_t> b;
    uint32_t p = 0;
    File(std::vector<uint8_t> x): b(x) {}
    bool seek(uint32_t offset) { if (offset > b.size()) return false; p=offset; return true; }
    uint32_t size() const {return b.size();}
    uint32_t position() const {return p;}
    int read() {return p<b.size()? b[p++]:-1;}
    size_t read(uint8_t *out, size_t count) {size_t n=0;while(n<count&&p<b.size())out[n++]=b[p++];return n;}
};
struct JDEC {void* device=nullptr;};
''' + probe + input_func + r'''
int main() {
    std::vector<uint8_t> h{0xff,0xd8,0xff,0xe0,0,4,'J','F',0xff,0xc0,0,11,8,0,10,0,10,1,1,0x11,0};
    File f(h);
    assert(jpegSofMarker(f)==0xc0 && f.position()==0);
    f.b[9]=0xc2;
    assert(jpegSofMarker(f)==0xc2 && f.position()==0);
    f.b[1]=0x00;
    assert(jpegSofMarker(f)==0 && f.position()==0);
    File content({10,20,30,40,50,60});
    JDEC decoder;decoder.device=&content;
    uint8_t data[3]{};
    assert(tinyJpegRead(&decoder,data,2)==2 && data[0]==10 && data[1]==20);
    assert(tinyJpegRead(&decoder,nullptr,2)==2 && content.position()==4);
    assert(tinyJpegRead(&decoder,data,2)==2 && data[0]==50 && data[1]==60);
    assert(tinyJpegRead(&decoder,nullptr,1)==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp)/'test.cpp'
    path.write_text(code)
    res = subprocess.run(['g++', '-std=c++17','-Wall','-Wextra','-Werror',str(path),'-o',str(Path(tmp)/'test')],capture_output=True,text=True)
    if res.returncode:
        raise SystemExit(res.stderr)
    subprocess.run([str(Path(tmp)/'test')],check=True)
print('JPEG marker probe and TJpg stream I/O: PASS')
