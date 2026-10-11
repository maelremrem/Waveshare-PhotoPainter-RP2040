import http from 'node:http';
import {readFile} from 'node:fs/promises';
import {dirname, extname, resolve, sep} from 'node:path';
import {fileURLToPath} from 'node:url';
const root=resolve(dirname(fileURLToPath(import.meta.url)), '..');
const port=Number(process.env.PORT)||8000;
const mime={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8',
  '.css':'text/css; charset=utf-8','.svg':'image/svg+xml','.png':'image/png',
  '.json':'application/json; charset=utf-8','.md':'text/plain; charset=utf-8'};
http.createServer(async(req,res)=>{
  let pathname;
  try{pathname=decodeURIComponent(new URL(req.url,'http://localhost').pathname);}catch{res.writeHead(400).end();return;}
  if(pathname.endsWith('/'))pathname+='index.html';
  const target=resolve(root,'.'+pathname);
  if(target!==root&&!target.startsWith(root+sep)){res.writeHead(403).end();return;}
  try{const file=await readFile(target);res.writeHead(200,{'Content-Type':mime[extname(target)]||'application/octet-stream','Cache-Control':'no-store'}).end(file);}
  catch{res.writeHead(404).end('Not found');}
}).listen(port,'127.0.0.1',()=>console.log(`PhotoPainter Studio → http://localhost:${port}/`));
