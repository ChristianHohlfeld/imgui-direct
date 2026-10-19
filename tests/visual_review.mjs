import { chromium } from 'playwright';
import fs from 'node:fs';
const url = process.env.DEMO_URL || 'https://christianhohlfeld.github.io/imgui-direct/';
fs.mkdirSync('visual-review',{recursive:true});
const browser = await chromium.launch({headless:true,args:[
  '--use-gl=angle','--use-angle=swiftshader','--enable-webgl',
  '--enable-unsafe-swiftshader','--disable-dev-shm-usage']});
let failures=[];
async function capture(name, width, height, deviceScaleFactor, mobile) {
 const page=await browser.newPage({viewport:{width,height},deviceScaleFactor,isMobile:mobile,hasTouch:mobile});
 const errors=[];
 page.on('pageerror',e=>errors.push('page: '+e.message));
 page.on('console',m=>{if(m.type()==='error')errors.push('console: '+m.text())});
 const response=await page.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
 await page.waitForTimeout(6500);
 await page.screenshot({path:'visual-review/'+name+'.png'});
 await page.waitForFunction(() => {
   const s=window.__imguiCanvasPixels?.state;
   return s?.ready && s.pixelWidth>0 && s.pixelHeight>0;
 },{timeout:12000});
 // Let ResizeObserver run and the C++ renderer reach a steady GL viewport.
 await page.waitForTimeout(200);
 const state=await page.evaluate(()=>{
   const canvas=document.getElementById('canvas');
   const status=document.getElementById('status')?.textContent||'';
   const metrics=window.__imguiCanvasPixels?.sync();
   const gl=canvas?.getContext('webgl2');
   const bounds=canvas?.getBoundingClientRect();
   const viewport=gl?.getParameter(gl.VIEWPORT);
   return {
     status, gl:!!gl, metrics, canvasWidth:canvas?.width,canvasHeight:canvas?.height,
     drawingWidth:gl?.drawingBufferWidth,drawingHeight:gl?.drawingBufferHeight,
     viewportWidth:viewport?.[2],viewportHeight:viewport?.[3],
     rectWidth:bounds?.width,rectHeight:bounds?.height,dpr:window.devicePixelRatio
   };
 });
 console.log(name,JSON.stringify({http:response?.status(),...state,errors}));
 if(!state.gl || state.status || errors.length)failures.push(name+' status '+JSON.stringify(state));
 if(state.canvasWidth !== state.metrics?.pixelWidth ||
    state.canvasHeight !== state.metrics?.pixelHeight ||
    state.canvasWidth !== state.drawingWidth ||
    state.canvasHeight !== state.drawingHeight ||
    state.viewportWidth !== state.drawingWidth ||
    state.viewportHeight !== state.drawingHeight)
   failures.push(name+' 1px physical mismatch: '+JSON.stringify(state));
 // Modern Chromium supports the authoritative device-pixel-content-box path.
 if(!state.metrics?.exact)failures.push(name+' uses rounded DPR instead of exact content box');
 if(name==='desktop') {
  await page.mouse.click(730,136);
  await page.waitForTimeout(900);
  await page.screenshot({path:'visual-review/desktop-elements.png'});
 }
 if(name==='mobile') {
  await page.touchscreen.tap(195,132);
  await page.waitForTimeout(1000);
  await page.screenshot({path:'visual-review/mobile-elements.png'});
 }
 await page.close();
}
try {
 await capture('desktop',1440,900,1,false);
 await capture('mobile',390,844,2,true);
 await capture('ipad',820,1180,2,true);
 // DPI matrix: integer, fractional Windows UI scaling and high-density phones.
 await capture('dpi125',1235,777,1.25,false);
 await capture('dpi150',1111,803,1.5,false);
 await capture('dpi133',1001,713,1.3333333333,false);
 await capture('dpi300',393,852,3,true);
 // Resize the same GL context: update size, actual buffer and viewport together.
 const resize=await browser.newPage({viewport:{width:801,height:601},deviceScaleFactor:1.25});
 try {
    await resize.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
    await resize.waitForTimeout(1800);
    await resize.setViewportSize({width:1003,height:709});
    await resize.waitForTimeout(800);
    const pixels=await resize.evaluate(()=>{
      const e=document.getElementById('canvas');
      const gl=e.getContext('webgl2');
      const r=window.__imguiCanvasPixels.sync();
      return {canvas:[e.width,e.height],
        backing:[gl.drawingBufferWidth,gl.drawingBufferHeight],
        glViewport:Array.from(gl.getParameter(gl.VIEWPORT)).slice(2),
        requested:[r.pixelWidth,r.pixelHeight],exact:r.exact};
    });
    console.log('resize',JSON.stringify(pixels));
    const p=pixels;
    if(!p.exact || p.canvas.some((value,i)=>
       value!==p.requested[i] || value!==p.backing[i] || value!==p.glViewport[i]))
      failures.push('resize physical pixel invariant: '+JSON.stringify(p));
    await resize.screenshot({path:'visual-review/resized.png'});
 } finally {await resize.close(); }
 for(const item of [{name:'daisy-button',url:'https://daisyui.com/components/button/'},
                    {name:'daisy-card',url:'https://daisyui.com/components/card/'}]) {
   const page=await browser.newPage({viewport:{width:1280,height:900}});
   await page.goto(item.url,{waitUntil:'domcontentloaded',timeout:45000});
   await page.waitForTimeout(1500);
   await page.screenshot({path:'visual-review/'+item.name+'.png'});
   await page.close();
 }
} finally {await browser.close();}
if(failures.length){console.error(failures.join('\n'));process.exitCode=1;}
