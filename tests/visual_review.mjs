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
 const state=await page.evaluate(()=>{
  const canvas=document.getElementById('canvas');
  const status=document.getElementById('status')?.textContent||'';
  const gl=canvas?.getContext('webgl2');
  return {status,gl:!!gl,canvasWidth:canvas?.width,canvasHeight:canvas?.height};
 });
 console.log(name,JSON.stringify({status:response?.status(),...state,errors}));
 if(!state.gl || (state.status && !state.status.includes('Loading')))failures.push(name+' '+JSON.stringify(state));
 if(name==='desktop') {
  await page.mouse.click(730,125);
  await page.waitForTimeout(900);
  await page.screenshot({path:'visual-review/desktop-elements.png'});
 }
 if(name==='mobile') {
  await page.touchscreen.tap(195,138);
  await page.waitForTimeout(1000);
  await page.screenshot({path:'visual-review/mobile-elements.png'});
 }
 await page.close();
}
try {
 await capture('desktop',1440,900,1,false);
 await capture('mobile',390,844,2,true);
 await capture('ipad',820,1180,2,true);
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
