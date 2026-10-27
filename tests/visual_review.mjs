import { chromium } from 'playwright';
import fs from 'node:fs';
import assert from 'node:assert/strict';
const url=process.env.DEMO_URL||'https://christianhohlfeld.github.io/imgui-direct/';
fs.mkdirSync('visual-review',{recursive:true});
const browser=await chromium.launch({headless:true,...(process.env.CHROMIUM_PATH?{executablePath:process.env.CHROMIUM_PATH}:{}),args:['--use-gl=angle','--use-angle=swiftshader','--enable-webgl','--enable-unsafe-swiftshader','--disable-dev-shm-usage']});
async function clickItem(page,id,touch=false){
  const r=await page.evaluate(id=>window.__imguiItems[id],id);assert(r,`Missing item ${id}`);
  const x=r.x+r.w/2,y=r.y+r.h/2;
  if(touch){const cdp=await page.context().newCDPSession(page);
    try{await cdp.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x,y}]});await page.waitForTimeout(80);await cdp.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});}
    finally{await cdp.detach();}
  }else{await page.mouse.move(x,y);await page.mouse.down();await page.waitForTimeout(80);await page.mouse.up();}
}
try{
  for(const [name,width,height,dpr,touch] of [['desktop',1440,1000,1,false],['mobile',390,844,2,true],['ipad',820,1180,2,true],['dpi125',1235,777,1.25,false],['dpi150',1111,803,1.5,false]]){
    const page=await browser.newPage({viewport:{width,height},deviceScaleFactor:dpr,isMobile:touch,hasTouch:touch});
    const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
    const response=await page.goto(url,{waitUntil:'domcontentloaded'});assert.equal(response.status(),200);
    await page.waitForFunction(()=>window.__imguiItems?.[100]&&document.getElementById('status').textContent==='');
    const initial=await page.evaluate(()=>window.__imguiUIState.clicks);
    await clickItem(page,100,touch);
    await page.waitForFunction(n=>window.__imguiUIState.clicks===n+1,initial);
    // Exercise every category from its actual C++ bounds, not brittle pixel constants.
    for(let category=0;category<8;++category){
      await clickItem(page,category,touch);
      await page.waitForFunction(n=>window.__imguiUIState.page===n,category);
      await page.waitForTimeout(200);
      if(name==='desktop'||name==='mobile')await page.screenshot({path:`visual-review/${name}-${category}.png`});
      if(category===1&&touch){
        const region=await page.evaluate(()=>window.__imguiInputRegions?.[1]);assert(region,'Second text input must register for IME');
        await page.touchscreen.tap(region.x+20,region.y+region.height/2);
        await page.waitForTimeout(200);
        assert(await page.evaluate(()=>document.activeElement===document.getElementById('native-input')),'IME bridge must focus the selected input');
        const before=await page.evaluate(()=>window.__imguiFields[1]);
        await page.evaluate(()=>{const e=document.getElementById('native-input');e.value='Ä';e.dispatchEvent(new Event('input',{bubbles:true}));});
        await page.waitForFunction(before=>window.__imguiFields[1]!==before&&window.__imguiFields[1].includes('Ä'),before);
        assert.equal(await page.evaluate(()=>window.__imguiUIState.page),1,'IME focus must preserve category');
        await page.evaluate(()=>document.getElementById('native-input').blur());
      }
      if(category===3){
        const before=await page.evaluate(()=>window.__imguiScrollTotal||0);
        await page.mouse.move(width/2,height-90);await page.mouse.wheel(0,90);await page.waitForTimeout(250);
        const stats=await page.evaluate(()=>({rows:window.__imguiScroll.rowsRendered,moved:(window.__imguiScrollTotal||0)}));
        assert(stats.rows>0&&stats.rows<100,`Virtual list should clip: ${JSON.stringify(stats)}`);
        assert(stats.moved>before,'Virtual list must scroll');
      }
    }
    await clickItem(page,0,touch);await page.waitForTimeout(150);
    const png=await page.screenshot({path:`visual-review/${name}.png`});
    const metrics=await page.evaluate(()=>{
      const c=document.getElementById('canvas'),gl=c.getContext('webgl2');return {canvas:[c.width,c.height],buffer:[gl.drawingBufferWidth,gl.drawingBufferHeight],viewport:[...gl.getParameter(gl.VIEWPORT)].slice(2)};
    });
    const physical=[png.readUInt32BE(16),png.readUInt32BE(20)];
    assert.deepEqual(metrics.canvas,metrics.buffer);assert.deepEqual(metrics.canvas,metrics.viewport);assert.deepEqual(metrics.canvas,physical);
    assert.equal(await page.locator('button').count(),0,'Components must be C++, not DOM replicas');
    assert.deepEqual(errors,[],`${name} runtime errors`);console.log(name,JSON.stringify(metrics),'all 8 categories passed');
    await page.close();
  }
}finally{await browser.close();}
