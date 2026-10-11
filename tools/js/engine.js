/* PhotoPainter Studio – 7-color ACeP rendering engine.
 * The exported image always uses exactly the physical seven-color palette.
 * Dependency-free and testable under Node.js and in a Web Worker.
 */
export const WIDTH = 800;
export const HEIGHT = 480;
export const PALETTE = Object.freeze([
  [0, 0, 0], [255, 255, 255], [0, 255, 0], [0, 0, 255],
  [255, 0, 0], [255, 255, 0], [255, 128, 0]
]);
export const PALETTE_LABELS = ['Noir', 'Blanc', 'Vert', 'Bleu', 'Rouge', 'Jaune', 'Orange'];
export const MONO_MODES = Object.freeze({
  off: 'Sept couleurs',
  photo: 'N&B photographie',
  soft: 'N&B doux',
  contrast: 'N&B contrasté',
  threshold: 'N&B seuil / encre'
});
export const paletteFor = config => config.monoMode && config.monoMode !== 'off' ? PALETTE.slice(0,2) : PALETTE;

export const DEFAULTS = Object.freeze({
  fit: 'contain', orientation: 0, background: '#ffffff', zoom:1, panX:0, panY:0,
  monoMode: 'off', monoBlackPoint: 0, monoWhitePoint: 255, monoThreshold: 128, monoInvert: false,
  algorithm: 'floyd', distance: 'oklab', strength: 0.90, grainPenalty: 0.14,
  serpentine: true, autoLevels: false, exposure: 0, gamma: 1.14,
  contrast: 1.04, saturation: 1.06, vibrance: 0, temperature: 0,
  tint: 0, shadows: 12, highlights: 0, sharpen: 0.45, denoise: 0
});
export const PRESETS = Object.freeze({
  natural: { monoMode:'off', algorithm: 'floyd', distance: 'oklab', strength: .85, grainPenalty: .14,
    autoLevels: false, exposure: 0, gamma: 1.14, contrast: 1.04, saturation: 1.06,
    vibrance: 0, temperature: 0, tint: 0, shadows: 12, highlights: 0, sharpen: .45, denoise: 0 },
  soft: { monoMode:'off', algorithm: 'atkinson', distance: 'oklab', strength: .68, grainPenalty: .22,
    autoLevels: false, exposure: 0, gamma: 1.20, contrast: .95, saturation: .94,
    vibrance: -8, temperature: 0, tint: 0, shadows: 16, highlights: -12, sharpen: .25, denoise: .15 },
  vivid: { monoMode:'off', algorithm: 'stucki', distance: 'oklab', strength: 1, grainPenalty: .08,
    autoLevels: true, exposure: 0, gamma: 1.07, contrast: 1.22, saturation: 1.28,
    vibrance: 16, temperature: 0, tint: 0, shadows: 8, highlights: -5, sharpen: .7, denoise: 0 },
  illustration: { monoMode:'off', algorithm: 'none', distance: 'oklab', strength: 0, grainPenalty: 0,
    autoLevels: false, exposure: 0, gamma: 1, contrast: 1.16, saturation: 1.15,
    vibrance: 0, temperature: 0, tint: 0, shadows: 0, highlights: 0, sharpen: .85, denoise: 0 },
  ordered: { monoMode:'off', algorithm: 'bayer8', distance: 'oklab', strength: .90, grainPenalty: .23,
    autoLevels: false, exposure: 0, gamma: 1.18, contrast: 1.06, saturation: 1.06,
    vibrance: 0, temperature: 0, tint: 0, shadows: 10, highlights: 0, sharpen: .3, denoise: 0 },
  'bw-photo': { monoMode:'photo', monoBlackPoint:0, monoWhitePoint:255, monoInvert:false, algorithm:'floyd',
    distance:'oklab', strength:.85, serpentine:true, exposure:0, gamma:1.08, contrast:1.07, saturation:1,
    vibrance:0, temperature:0, tint:0, shadows:8, highlights:0, sharpen:.45, denoise:.10, autoLevels:false },
  'bw-soft': { monoMode:'soft', monoBlackPoint:0, monoWhitePoint:255, monoInvert:false, algorithm:'atkinson',
    distance:'oklab', strength:.75, serpentine:true, exposure:0, gamma:1.12, contrast:.95, saturation:1,
    vibrance:0, temperature:0, tint:0, shadows:12, highlights:-5, sharpen:.2, denoise:.15, autoLevels:false },
  'bw-contrast': { monoMode:'contrast', monoBlackPoint:5, monoWhitePoint:250, monoInvert:false, algorithm:'floyd',
    distance:'oklab', strength:1, serpentine:true, exposure:0, gamma:1, contrast:1.1, saturation:1,
    vibrance:0, temperature:0, tint:0, shadows:0, highlights:0, sharpen:.5, denoise:0, autoLevels:false },
  'bw-ink': { monoMode:'threshold', monoBlackPoint:0, monoWhitePoint:255, monoThreshold:128, monoInvert:false,
    algorithm:'none', distance:'oklab', strength:0, exposure:0, gamma:1, contrast:1.1, saturation:1,
    vibrance:0, temperature:0, tint:0, shadows:0, highlights:0, sharpen:.8, denoise:0, autoLevels:false }
});
export const ALGORITHMS = Object.freeze({
  none: 'Sans dithering', floyd: 'Floyd–Steinberg', atkinson: 'Atkinson',
  'sierra-lite': 'Sierra Lite', burkes: 'Burkes', stucki: 'Stucki',
  jjn: 'Jarvis–Judice–Ninke', bayer4: 'Bayer 4×4', bayer8: 'Bayer 8×8',
  bayer16: 'Bayer 16×16', noise: 'Bruit réparti'
});
const KERNELS = {
  floyd: { divisor: 16, points: [[1,0,7],[-1,1,3],[0,1,5],[1,1,1]] },
  atkinson: { divisor: 8, points: [[1,0,1],[2,0,1],[-1,1,1],[0,1,1],[1,1,1],[0,2,1]] },
  'sierra-lite': { divisor: 4, points: [[1,0,2],[-1,1,1],[0,1,1]] },
  burkes: { divisor: 32, points: [[1,0,8],[2,0,4],[-2,1,2],[-1,1,4],[0,1,8],[1,1,4],[2,1,2]] },
  stucki: { divisor: 42, points: [[1,0,8],[2,0,4],[-2,1,2],[-1,1,4],[0,1,8],[1,1,4],[2,1,2],[-2,2,1],[-1,2,2],[0,2,4],[1,2,2],[2,2,1]] },
  jjn: { divisor: 48, points: [[1,0,7],[2,0,5],[-2,1,3],[-1,1,5],[0,1,7],[1,1,5],[2,1,3],[-2,2,1],[-1,2,3],[0,2,5],[1,2,3],[2,2,1]] }
};
const clamp = (v, lo = 0, hi = 255) => Math.max(lo, Math.min(hi, v));
const srgb = v => { const a = clamp(v, 0, 255)/255; return a <= .04045 ? a/12.92 : ((a+.055)/1.055)**2.4; };
function toLab(r,g,b) {
  r=srgb(r);g=srgb(g);b=srgb(b);
  const l=Math.cbrt(.4122214708*r+.5363325363*g+.0514459929*b);
  const m=Math.cbrt(.2119034982*r+.6806995451*g+.1073969566*b);
  const s=Math.cbrt(.0883024619*r+.2817188376*g+.6299787005*b);
  return [.2104542553*l+.7936177850*m-.0040720468*s,
    1.9779984951*l-2.4285922050*m+.4505937099*s,
    .0259040371*l+.7827717662*m-.8086757660*s];
}
const PALETTE_LAB = PALETTE.map(p=>toLab(...p));
function coords(r,g,b,metric) {
  if (metric==='oklab') return toLab(r,g,b);
  if (metric==='weighted') return [r*.85,g*1.12,b*.74];
  return [r,g,b];
}
function paletteCoords(metric, colors=PALETTE) {return metric==='oklab'? (colors===PALETTE?PALETTE_LAB:colors.map(p=>toLab(...p))) : colors.map(p=>coords(...p,metric));}
function nearest(v, palette) {
  let best=0, min=Infinity;
  for(let j=0;j<palette.length;j++) {
    const p=palette[j], d0=v[0]-p[0],d1=v[1]-p[1],d2=v[2]-p[2];
    const d=d0*d0+d1*d1+d2*d2;
    if(d<min){min=d;best=j;}
  }
  return best;
}
function bayer(n) {
  let a=[[0]];
  while(a.length<n) {
    const w=a.length,b=Array.from({length:w*2},()=>new Array(w*2));
    for(let y=0;y<w;y++)for(let x=0;x<w;x++){
      const v=a[y][x]*4;
      b[y][x]=v; b[y][x+w]=v+2;b[y+w][x]=v+3;b[y+w][x+w]=v+1;
    }
    a=b;
  }
  return a;
}
const BAYER={4:bayer(4),8:bayer(8),16:bayer(16)};
function pseudoRandom(x,y) {
  let t=Math.imul(x+1,374761393)+Math.imul(y+1,668265263);
  t=Math.imul(t^(t>>>13),1274126177);
  return ((t^(t>>>16))>>>0)/4294967296;
}
export function applyAdjustments(image, config) {
  const w=image.width, h=image.height;
  const raw=image.data, out=new Float32Array(w*h*3);
  const exposure=2**(+config.exposure||0),gamma=+config.gamma||1,
    contrast=+config.contrast||1,saturation=+config.saturation||0,
    vibrance=(+config.vibrance||0)/100, temp=(+config.temperature||0)/100,
    tint=(+config.tint||0)/100, sh=(+config.shadows||0)/100, hi=(+config.highlights||0)/100;
  let lo=0,hiLevel=255;
  if(config.autoLevels) {
    const hist=new Uint32Array(256);
    for(let i=0;i<raw.length;i+=4)hist[Math.round(.2126*raw[i]+.7152*raw[i+1]+.0722*raw[i+2])]++;
    const threshold=w*h*.008;
    let sum=0;
    for(let i=0;i<256;i++){sum+=hist[i];if(sum>=threshold){lo=i;break;}}
    sum=0;
    for(let i=255;i>=0;i--){sum+=hist[i];if(sum>=threshold){hiLevel=i;break;}}
    if(hiLevel-lo<35){lo=0;hiLevel=255;}
  }
  for(let i=0,k=0;i<raw.length;i+=4,k+=3) {
    let r=raw[i],g=raw[i+1],b=raw[i+2];
    if(config.autoLevels){const q=255/(hiLevel-lo);r=clamp((r-lo)*q);g=clamp((g-lo)*q);b=clamp((b-lo)*q);}
    r=clamp(r*exposure);g=clamp(g*exposure);b=clamp(b*exposure);
    let luma=(r*.2126+g*.7152+b*.0722)/255;
    const shadows=(1-luma)**2 * sh * 70, highlights=luma*luma * hi * 70;
    r=clamp(r+shadows+highlights);g=clamp(g+shadows+highlights);b=clamp(b+shadows+highlights);
    r=clamp((r-127.5)*contrast+127.5);g=clamp((g-127.5)*contrast+127.5);b=clamp((b-127.5)*contrast+127.5);
    r=255*(r/255)**(1/gamma);g=255*(g/255)**(1/gamma);b=255*(b/255)**(1/gamma);
    r=clamp(r+temp*24+tint*13);g=clamp(g-tint*22);b=clamp(b-temp*24+tint*13);
    luma=.2126*r+.7152*g+.0722*b;
    const sat0=(Math.max(r,g,b)-Math.min(r,g,b))/255;
    const satGain=clamp(saturation + vibrance*(1-sat0),0,3);
    out[k]=clamp(luma+(r-luma)*satGain);out[k+1]=clamp(luma+(g-luma)*satGain);out[k+2]=clamp(luma+(b-luma)*satGain);
  }
  const dn=+config.denoise||0;
  if(dn>.01){
    const copy=out.slice();
    for(let y=1;y<h-1;y++)for(let x=1;x<w-1;x++){
      const q=(y*w+x)*3;
      for(let ch=0;ch<3;ch++){
        const v=(copy[q+ch]*4+copy[q-3+ch]+copy[q+3+ch]+copy[q-w*3+ch]+copy[q+w*3+ch])/8;
        out[q+ch]=copy[q+ch]*(1-dn)+v*dn;
      }
    }
  }
  const sharp=+config.sharpen||0;
  if(sharp>.01){
    const copy=out.slice(); const coeff=sharp*.42;
    for(let y=1;y<h-1;y++)for(let x=1;x<w-1;x++){
      const q=(y*w+x)*3;
      for(let ch=0;ch<3;ch++){
        const v=copy[q+ch]*4-copy[q-3+ch]-copy[q+3+ch]-copy[q-w*3+ch]-copy[q+w*3+ch];
        out[q+ch]=clamp(copy[q+ch]+coeff*v);
      }
    }
  }
  return out;
}
// A monochrome frame uses actual black and white ink, not synthetic grey pixels.
// Grey tones are represented spatially by the selected dithering algorithm.
export function convertToMonochrome(src, config) {
  const mode=config.monoMode||'off';
  if(mode==='off')return src;
  const black=clamp(Number(config.monoBlackPoint ?? 0),0,250);
  const white=clamp(Number(config.monoWhitePoint ?? 255),black+1,255);
  const threshold=clamp(Number(config.monoThreshold ?? 128),0,255);
  const inverted=!!config.monoInvert;
  for(let i=0;i<src.length;i+=3) {
    const y=src[i]*.2126+src[i+1]*.7152+src[i+2]*.0722;
    let tone=clamp((y-black)*255/(white-black));
    if(mode==='soft')tone=255*(tone/255)**(1/1.12);
    else if(mode==='contrast')tone=clamp((tone-127.5)*1.55+127.5);
    else if(mode==='threshold')tone=tone>=threshold?255:0;
    if(inverted)tone=255-tone;
    src[i]=tone;src[i+1]=tone;src[i+2]=tone;
  }
  return src;
}
export function quantize(image, config) {
  const {width:w,height:h}=image;
  const src=convertToMonochrome(applyAdjustments(image,config),config);
  const dst=new Uint8ClampedArray(w*h*4);
  const metric=config.distance||'oklab';
  const colors=paletteFor(config);
  const panel=paletteCoords(metric,colors);
  const algo=config.monoMode==='threshold'?'none':(config.algorithm||'floyd');
  const strength=clamp(Number(config.strength ?? 0),0,1);
  if(KERNELS[algo]) {
    const work=new Float32Array(src); const {points,divisor}=KERNELS[algo];
    for(let y=0;y<h;y++) {
      const reverse=!!config.serpentine&&(y&1);
      for(let t=0;t<w;t++) {
        const x=reverse?w-1-t:t, q=(y*w+x)*3;
        const r=clamp(work[q]),g=clamp(work[q+1]),b=clamp(work[q+2]);
        const idx=nearest(coords(r,g,b,metric),panel), col=colors[idx];
        const k=(y*w+x)*4;
        dst[k]=col[0];dst[k+1]=col[1];dst[k+2]=col[2];dst[k+3]=255;
        const er=(r-col[0])*strength/divisor,eg=(g-col[1])*strength/divisor,eb=(b-col[2])*strength/divisor;
        for(const [dx,dy,weight] of points){
          const xx=x+(reverse?-dx:dx), yy=y+dy;
          if(xx<0||xx>=w||yy>=h)continue;
          const z=(yy*w+xx)*3;
          work[z]+=er*weight;work[z+1]+=eg*weight;work[z+2]+=eb*weight;
        }
      }
    }
    return {width:w,height:h,data:dst};
  }
  const n=algo.startsWith('bayer')? Number(algo.slice(5)):0;
  const grid=BAYER[n], grain=+config.grainPenalty||0;
  for(let y=0;y<h;y++) for(let x=0;x<w;x++) {
    const i=y*w+x, k=i*3, p=coords(src[k],src[k+1],src[k+2],metric);
    let idx;
    if(!grid && algo!=='noise'||strength<=0){idx=nearest(p,panel);}
    else {
      let first=0,second=0,mix=0,cost=Infinity;
      for(let a=0;a<colors.length;a++)for(let b=a;b<colors.length;b++){
        const va=panel[a], vb=panel[b];
        const d0=vb[0]-va[0], d1=vb[1]-va[1], d2=vb[2]-va[2];
        const len=d0*d0+d1*d1+d2*d2;
        const alpha=len>.0000001?clamp(((p[0]-va[0])*d0+(p[1]-va[1])*d1+(p[2]-va[2])*d2)/len,0,1):0;
        const e0=p[0]-va[0]-alpha*d0,e1=p[1]-va[1]-alpha*d1,e2=p[2]-va[2]-alpha*d2;
        const score=e0*e0+e1*e1+e2*e2+grain*alpha*(1-alpha)*len;
        if(score<cost){cost=score;first=a;second=b;mix=alpha;}
      }
      const probability=mix*strength+(mix>=.5?1-strength:0);
      const threshold=grid?(grid[y%n][x%n]+.5)/(n*n):pseudoRandom(x,y);
      idx=probability>threshold?second:first;
    }
    const c=colors[idx], z=i*4;
    dst[z]=c[0];dst[z+1]=c[1];dst[z+2]=c[2];dst[z+3]=255;
  }
  return {width:w,height:h,data:dst};
}
export function bmp24(image){
  const {width:w,height:h,data}=image,row=((w*3+3)&~3),size=54+row*h;
  const buf=new ArrayBuffer(size),v=new DataView(buf),out=new Uint8Array(buf);
  out[0]=66;out[1]=77;v.setUint32(2,size,true);v.setUint32(10,54,true);
  v.setUint32(14,40,true);v.setInt32(18,w,true);v.setInt32(22,h,true);
  v.setUint16(26,1,true);v.setUint16(28,24,true);v.setUint32(34,size-54,true);
  for(let y=0;y<h;y++){
    let dst=54+(h-y-1)*row;
    for(let x=0;x<w;x++){
      const src=(y*w+x)*4;
      out[dst++]=data[src+2];out[dst++]=data[src+1];out[dst++]=data[src];
    }
  }
  return new Uint8Array(buf);
}
export function outputSize(orientation){const portrait=(+orientation===90||+orientation===270);return {width:portrait?HEIGHT:WIDTH,height:portrait?WIDTH:HEIGHT};}
export function sanitizeBaseName(name){const base=name.replace(/\.[^.]+$/,'').normalize('NFKD').replace(/[\u0300-\u036f]/g,'').replace(/[^a-zA-Z0-9_-]+/g,'_').slice(0,64);return base||'photo';}
