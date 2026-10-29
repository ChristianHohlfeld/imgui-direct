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
  }else{await page.mouse.move(x,y);await page.waitForTimeout(50);await page.mouse.down();await page.waitForTimeout(80);await page.mouse.up();}
}
try{
  for(const [name,width,height,dpr,touch] of [['desktop',1440,1000,1,false],['mobile',390,844,2,true],['ipad',820,1180,2,true],['dpi125',1235,777,1.25,false],['dpi150',1111,803,1.5,false]]){
    const page=await browser.newPage({viewport:{width,height},deviceScaleFactor:dpr,isMobile:touch,hasTouch:touch});
    const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
    const response=await page.goto(url,{waitUntil:'domcontentloaded'});assert.equal(response.status(),200);
    await page.waitForFunction(()=>window.__imguiItems?.[8]&&document.getElementById('status').textContent==='');
    // The actual first page is the browseable component catalogue.
    // Capture the real rendered canvas BEFORE the legacy example categories.
    assert.equal(await page.evaluate(()=>window.__imguiUIState?.page),9);
    await page.waitForTimeout(250);
    if(name==='desktop'||name==='mobile'||name==='ipad')
      await page.screenshot({path:`visual-review/${name}-catalog-light.png`});
    const initialGallery=await page.evaluate(()=>window.__imguiGallery);
    assert.equal(initialGallery.matching,52,'All 52 catalogue entries must be discoverable');
    assert(initialGallery.drawn>0&&initialGallery.drawn<52,
      'Gallery must virtualize invisible preview cards: '+JSON.stringify(initialGallery));
    if(name==='desktop'){
      await clickItem(page,1004);
      await page.waitForFunction(()=>window.__imguiGallery?.filter===4&&window.__imguiGallery.matching>0);
      await page.screenshot({path:'visual-review/desktop-catalog-filtered.png'});
      await clickItem(page,1000);
      await page.waitForFunction(()=>window.__imguiGallery?.filter===0);
      // Screenshot real list scrolling in the catalogue, not just the 100k-row demo.
      await page.mouse.move(width*.63,height*.7);
      await page.mouse.wheel(0,675);
      await page.waitForTimeout(200);
      await page.screenshot({path:'visual-review/desktop-catalog-scrolled.png'});
      await page.mouse.wheel(0,-2600);
      await page.waitForTimeout(300);
    }
    if(name==='desktop'){
      await clickItem(page,910,false);
      await page.waitForTimeout(150);
      await page.screenshot({path:'visual-review/desktop-catalog-dark.png'});
      await clickItem(page,910,false);
      await page.waitForTimeout(150);
      // Card action must remain visible and clickable even when the
      // description wraps and the gallery has been scrolled.
      await page.waitForFunction(()=>{
        const r=window.__imguiItems?.[900];
        return r&&r.y>140&&r.y<880&&r.h>5;
      });
      await clickItem(page,900,false);
      await page.waitForFunction(()=>window.__imguiUIState?.page===8);
      await clickItem(page,0,false);
      await page.waitForFunction(()=>window.__imguiUIState?.page===9);
    }
    // Saturated page boundary has visible elastic feedback but clamped scroll.
    if(name==='desktop'){
      await page.mouse.move(width*.5,height*.7);
      await page.mouse.wheel(0,-500);
      await page.waitForFunction(()=>Math.abs(window.__imguiScrollDebug?.elasticY||0)>.5);
      const bounce=await page.evaluate(()=>window.__imguiScrollDebug.elasticY);
      assert(bounce<0,'Pull above page top must produce outward displacement');
      await page.waitForTimeout(700);
      const settled=await page.evaluate(()=>window.__imguiScrollDebug.elasticY);
      assert(Math.abs(settled)<Math.abs(bounce)*.15,
        'Elastic scroll must spring back after gesture, not stick at boundary');
    }
    await clickItem(page,0,touch);
    await page.waitForFunction(()=>window.__imguiItems?.[100]&&window.__imguiUIState.page===0);
    await page.waitForTimeout(200); // settle newly-created auto-resizing example cards
    const initial=await page.evaluate(()=>window.__imguiUIState.clicks);
    await clickItem(page,100,touch);
    await page.waitForFunction(n=>window.__imguiUIState.clicks===n+1,initial).catch(async error=>{
      await page.screenshot({path:`visual-review/${name}-action-failure.png`});
      console.log('action-failure',await page.evaluate(()=>({state:window.__imguiUIState,items:window.__imguiItems,errors:window.__imguiReview})));
      throw error;
    });
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
        assert(await page.evaluate(()=>document.activeElement===document.getElementById('native-input')),'IME bridge must focus the selected input: '+JSON.stringify(await page.evaluate(()=>({active:document.activeElement?.id,regions:window.__imguiInputRegions}))));
        const before=await page.evaluate(()=>window.__imguiFields[1]);
        await page.evaluate(()=>{const e=document.getElementById('native-input');e.value='Ä';e.dispatchEvent(new Event('input',{bubbles:true}));});
        await page.waitForFunction(before=>window.__imguiFields[1]!==before&&window.__imguiFields[1].includes('Ä'),before);
        assert.equal(await page.evaluate(()=>window.__imguiUIState.page),1,'IME focus must preserve category');
        await page.evaluate(()=>document.getElementById('native-input').blur());
      }
      if(category===3){
        const before=await page.evaluate(()=>window.__imguiScrollTotal||0);
        const list=await page.evaluate(()=>window.__imguiItems[102]);assert(list,'List bounds must be reported');
        await page.mouse.move(list.x+list.w/2,list.y+Math.min(list.h/2,80));
        await page.mouse.wheel(0,90);await page.waitForTimeout(250);
        const stats=await page.evaluate(()=>({rows:window.__imguiScroll.rowsRendered,moved:(window.__imguiScrollTotal||0)}));
        assert(stats.rows>0&&stats.rows<100,`Virtual list should clip: ${JSON.stringify(stats)}`);
        assert(stats.moved>before,'Virtual list must scroll');
        // Regression capture: after a real wheel gesture inside the 100k-row
        // native list, toolbar labels and unrelated action captions must
        // remain visible. Inspect this rendered PNG alongside catalog shots.
        if(name==='desktop'||name==='mobile')
          await page.screenshot({path:`visual-review/${name}-list-after-scroll.png`});
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
