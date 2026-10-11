import {quantize, outputSize} from './engine.js';
async function render(file, config){
  let bitmap;
  try { bitmap=await createImageBitmap(file,{imageOrientation:'from-image'}); }
  catch { bitmap=await createImageBitmap(file); }
  const {width,height}=outputSize(config.orientation);
  const canvas=new OffscreenCanvas(width,height);
  const ctx=canvas.getContext('2d',{willReadFrequently:true});
  if(!ctx)throw Error('Canvas 2D inaccessible');
  ctx.fillStyle=config.background||'#ffffff';ctx.fillRect(0,0,width,height);
  const iw=bitmap.width,ih=bitmap.height;
  const baseScale=(config.fit==='cover')?Math.max(width/iw,height/ih):Math.min(width/iw,height/ih);
  const scale=baseScale*Math.max(1,Math.min(3,Number(config.zoom)||1));
  const dw=iw*scale,dh=ih*scale;
  ctx.imageSmoothingEnabled=true;ctx.imageSmoothingQuality='high';
  const panX=Math.max(-1,Math.min(1,(Number(config.panX)||0)/100));
  const panY=Math.max(-1,Math.min(1,(Number(config.panY)||0)/100));
  const x=(width-dw)/2-Math.max(0,dw-width)*panX/2;
  const y=(height-dh)/2-Math.max(0,dh-height)*panY/2;
  ctx.drawImage(bitmap,x,y,dw,dh);
  bitmap.close();
  const image=ctx.getImageData(0,0,width,height);
  return quantize(image,config);
}
let tail=Promise.resolve();
self.addEventListener('message',event=>{
  const {id,file,config}=event.data;
  tail=tail.catch(()=>{}).then(async()=>{
    try {
      const result=await render(file,config);
      self.postMessage({id,type:'result',width:result.width,height:result.height,pixels:result.data.buffer},[result.data.buffer]);
    }catch(error){self.postMessage({id,type:'error',message:error?.message||String(error)});}
  });
});
