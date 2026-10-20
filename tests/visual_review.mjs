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
 const screenshot=await page.screenshot({path:'visual-review/'+name+'.png'});
 const outputPixels=[screenshot.readUInt32BE(16),screenshot.readUInt32BE(20)];
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
 if(await page.locator('button').count()!==0)
   failures.push(name+' HTML startup button bypasses native component system');
 if((await page.title())!=='Komponenten')
   failures.push(name+' technical browser title still exposed');
 if(state.canvasWidth !== state.metrics?.pixelWidth ||
    state.canvasHeight !== state.metrics?.pixelHeight ||
    state.canvasWidth !== state.drawingWidth ||
    state.canvasHeight !== state.drawingHeight ||
    state.viewportWidth !== state.drawingWidth ||
    state.viewportHeight !== state.drawingHeight)
   failures.push(name+' 1px physical mismatch: '+JSON.stringify(state));
 // Modern Chromium supports the authoritative device-pixel-content-box path.
 // A canvas backing store can agree with WebGL and still be WRONG:
 // compare to the independent Chromium screenshot/compositor pixel dimensions.
 if(state.canvasWidth!==outputPixels[0] || state.canvasHeight!==outputPixels[1])
   failures.push(name+' compositor rescaling detected: '+JSON.stringify({
     canvas:[state.canvasWidth,state.canvasHeight],png:outputPixels,
     dpr:state.dpr,source:state.metrics?.source
   }));
 // Exact native devicePixelContentBox when trustworthy; the fallback is
 // necessary on browsers/emulators which report CSS units from the observer.
 if(!state.metrics?.exact)console.log(name+' fallback: '+state.metrics?.source);
 // A button must respond to a real mobile touch tap and reject a swipe
 // that began over the same button. This probes the *full* browser -> touch
 // controller -> ImGui -> C++ component chain.
 if(name==='mobile'){
   await page.waitForFunction(()=>window.__imguiUIState?.page===0,null,{timeout:6000});
   const first=await page.evaluate(()=>window.__imguiUIState.clicks);
   await page.touchscreen.tap(105,374); // primary button, CSS-pixel coordinates
   await page.waitForFunction(initial=>window.__imguiUIState?.clicks===initial+1,
                              first,{timeout:4000});
   const clicked=await page.evaluate(()=>window.__imguiUIState.clicks);
   const session=await page.context().newCDPSession(page);
   try {
     await session.send('Input.dispatchTouchEvent',
       {type:'touchStart',touchPoints:[{x:105,y:374}]});
     await page.waitForTimeout(75);
     await session.send('Input.dispatchTouchEvent',
       {type:'touchMove',touchPoints:[{x:105,y:294}]});
     await page.waitForTimeout(100);
     await session.send('Input.dispatchTouchEvent',
       {type:'touchEnd',touchPoints:[]});
     await page.waitForTimeout(160);
     const end=await page.evaluate(()=>window.__imguiUIState.clicks);
     if(end!==clicked)failures.push('swipe incorrectly clicked native button');
     // Explicit OS touchCancel is NOT a finger release or a tap.
     await session.send('Input.dispatchTouchEvent',
       {type:'touchStart',touchPoints:[{x:105,y:374}]});
     await page.waitForTimeout(75);
     await session.send('Input.dispatchTouchEvent',
       {type:'touchCancel',touchPoints:[]});
     await page.waitForTimeout(160);
     const afterCancel=await page.evaluate(()=>window.__imguiUIState.clicks);
     if(afterCancel!==clicked)failures.push('touchCancel incorrectly clicked native button');
     console.log('touch-button',JSON.stringify({
       tap:clicked-first,swipeClicks:end-clicked,cancelClicks:afterCancel-clicked
     }));
   } finally {await session.detach();}
 }
 // Assert the DOM event's actual CSS delta, not Playwright's requested
 // PHYSICAL wheel distance (the two differ under simulated DPR > 1).
 if(name==='mobile'){
   await page.mouse.move(180,650);
   const start=await page.evaluate(()=>window.__imguiScrollTotal||0);
   await page.mouse.wheel(0,90);
   await page.waitForFunction(()=>window.__imguiRawWheel?.received>0,
                              null,{timeout:6000});
   const actual=await page.evaluate(()=>window.__imguiRawWheel.y);
   await page.waitForFunction(({start,actual})=>
     (window.__imguiScrollTotal||0)>=start+actual-1,
     {start,actual},{timeout:6000});
   let moved=await page.evaluate(start=>(window.__imguiScrollTotal||0)-start,start);
   console.log('real-wheel',JSON.stringify({physicalRequest:90,cssReceived:actual,moved}));
   if(Math.abs(moved-actual)>1.2)failures.push('real Chrome wheel delta mismatch: '+moved+' vs '+actual);

   // Explicitly emit a CSS-pixel precise WheelEvent to ensure 90 really
   // means 90 logical layout pixels (no double ImGui wheel scaling).
   const secondStart=await page.evaluate(()=>window.__imguiScrollTotal||0);
   await page.evaluate(()=>document.getElementById('canvas').dispatchEvent(
      new WheelEvent('wheel',{deltaX:0,deltaY:90,deltaMode:0,
                              clientX:180,clientY:650,bubbles:true,cancelable:true})));
   await page.waitForFunction(start=>
       (window.__imguiScrollTotal||0)>=start+89,
       secondStart,{timeout:6000});
   moved=await page.evaluate(start=>(window.__imguiScrollTotal||0)-start,secondStart);
   console.log('synthetic-css-wheel',JSON.stringify({expected:90,moved}));
   if(Math.abs(moved-90)>1.2)failures.push('1:1 DOM pixel delta mismatch: '+moved);
 }
 if(name==='desktop') {
  // The canvas-backed native button uses the OS hand cursor like Chrome UI.
  await page.mouse.move(255,375);
  await page.waitForFunction(()=>
    document.getElementById('canvas')?.style.cursor==='pointer',
    null,{timeout:5000});
  await page.mouse.move(1400,870);
  await page.waitForFunction(()=>
    document.getElementById('canvas')?.style.cursor==='default',
    null,{timeout:5000});
  await page.mouse.click(730,136);
  await page.waitForTimeout(900);
  await page.screenshot({path:'visual-review/desktop-elements.png'});
 }
 if(name==='mobile') {
  // Four tabs: x~145 selects Formulare; this page is scrollable too.
  await page.touchscreen.tap(143,122);
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
 // A touch-enabled laptop (iPad + trackpad-like mixed input) must
 // switch source back to Mouse on pointer movement after a finger tap.
 const hybrid=await browser.newPage({viewport:{width:390,height:844},
                                    deviceScaleFactor:2,hasTouch:true,isMobile:false});
 try{
   await hybrid.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
   await hybrid.waitForFunction(()=>window.__imguiUIState?.page===0,
                                null,{timeout:20000});
   await hybrid.touchscreen.tap(105,374);
   await hybrid.waitForTimeout(850); // suppress only synthesized touch mice
   await hybrid.mouse.move(380,800);
   await hybrid.waitForTimeout(80);
   await hybrid.mouse.move(105,374);
   try{
     await hybrid.waitForFunction(()=>
       document.getElementById('canvas')?.style.cursor==='pointer',
       null,{timeout:5000});
   }catch(err){
     console.log('hybrid hover debug',JSON.stringify(await hybrid.evaluate(()=>
        ({cursor:document.getElementById('canvas').style.cursor,
          pointer:window.__imguiPointerState, touches:navigator.maxTouchPoints,
          viewport:window.innerWidth}))));
     throw err;
   }
   await hybrid.mouse.move(380,800);
   await hybrid.waitForFunction(()=>
     document.getElementById('canvas')?.style.cursor==='default',
     null,{timeout:5000});
   console.log('hybrid-mouse-touch: hand cursor restored');
 }finally{await hybrid.close();}
 // The public 'Liste' tab has 100,000 records; the C++ clipper should
 // submit only the visible items and render immediately on a wheel delta.
 const listPage=await browser.newPage({viewport:{width:390,height:844},
                                      deviceScaleFactor:2,isMobile:true,hasTouch:true});
 try{
   await listPage.goto(url,{waitUntil:'domcontentloaded',timeout:45000});
   // Wait for the actual WASM-rendered C++ app, not a fixed delay. On a
   // cold software-WebGL runner startup can exceed 2.4 seconds, otherwise
   // this test clicks the blank loading canvas and reports a false failure.
   await listPage.waitForFunction(() => window.__imguiUIState?.page===0,
                                  null,{timeout:20000});
   // Hold through at least two frames so real ImGui mouse-down/up are
   // processed, rather than dropping a synthetic 1ms click on WebGL.
   await listPage.mouse.move(339,122);
   await listPage.mouse.down();
   await listPage.waitForTimeout(105);
   await listPage.mouse.up();
   try {
     await listPage.waitForFunction(()=>window.__imguiUIState?.page===3,
                                    null,{timeout:4500});
     await listPage.waitForFunction(()=>window.__imguiScroll?.rowsRendered>0,
                                    null,{timeout:4500});
   } catch(error) {
     console.log('list selection debug',JSON.stringify(
       await listPage.evaluate(()=>({ui:window.__imguiUIState,
           scroll:window.__imguiScroll, wheel:window.__imguiRawWheel}))));
     await listPage.screenshot({path:'visual-review/list-selection-failed.png'});
     throw error;
   }
   const before=await listPage.evaluate(()=>window.__imguiScrollTotal||0);
   const visible=await listPage.evaluate(()=>window.__imguiScroll?.rowsRendered);
   if(visible>55)failures.push('list virtualization submitted '+visible+' rows');
   await listPage.mouse.move(180,580);
   await listPage.mouse.wheel(0,117);
   await listPage.waitForFunction(()=>window.__imguiRawWheel?.received>0,
                                  null,{timeout:5000});
   const actualDelta=await listPage.evaluate(()=>window.__imguiRawWheel.y);
   await listPage.waitForFunction(({before,actualDelta})=>
       (window.__imguiScrollTotal||0)>=before+actualDelta-1,
       {before,actualDelta},{timeout:6000});
   const movement=await listPage.evaluate(start=>
       (window.__imguiScrollTotal||0)-start,before);
   if(Math.abs(movement-actualDelta)>1.2)
       failures.push('list precise wheel: '+movement+' vs '+actualDelta);
   await listPage.screenshot({path:'visual-review/virtual-list.png'});
   console.log('virtual-list',JSON.stringify({total:100000,visible,movement}));
 }finally{await listPage.close();}

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
    if(p.canvas.some((value,i)=>
       value!==p.requested[i] || value!==p.backing[i] || value!==p.glViewport[i]))
      failures.push('resize physical pixel invariant: '+JSON.stringify(p));
    const resizedPng=await resize.screenshot({path:'visual-review/resized.png'});
    const screenshotPhysical=[resizedPng.readUInt32BE(16),resizedPng.readUInt32BE(20)];
    if(p.canvas.some((pixel,i)=>pixel!==screenshotPhysical[i]))
       failures.push('resize screenshot/resolution mismatch: '+JSON.stringify({
         frame:p.canvas,png:screenshotPhysical
       }));
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
