import {chromium} from 'playwright';
import fs from 'node:fs';
import assert from 'node:assert/strict';
const url=process.env.DEMO_URL||'https://christianhohlfeld.github.io/imgui-direct/';
const root='visual-review/states';fs.mkdirSync(root,{recursive:true});
const states=['normal','selected','disabled','readonly','loading','error','warning','success','empty','mixed','open','closed','minimum','maximum','long','required'];
const browser=await chromium.launch({headless:true,args:['--use-gl=angle','--use-angle=swiftshader','--enable-webgl','--enable-unsafe-swiftshader','--disable-dev-shm-usage']});
const manifest={catalog:null,renders:[],interactions:[]};
async function configure(page,component,state,dark,size=2){
 await page.evaluate(({component,state,dark,size})=>{
   document.getElementById('native-input').blur();window.__reviewRequested=performance.now();
   Module.ccall('gallery_review',null,['number','number','number','number'],[component,state,dark,size]);
 },{component,state,dark,size});
 await page.waitForFunction(({component,state})=>window.__imguiReview?.component===component&&window.__imguiReview.state===state&&window.__imguiReview.time>window.__reviewRequested,{component,state});
}
async function pointer(page,rect,touch,downOnly=false){
 const x=rect.x+Math.min(rect.w*.5,35),y=rect.y+rect.h*.5;
 if(touch){
  const session=await page.context().newCDPSession(page);
  try{await session.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x,y}]});await page.waitForTimeout(65);if(!downOnly)await session.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});}
  finally{await session.detach();}
 }else{await page.mouse.move(x,y);await page.mouse.down();await page.waitForTimeout(65);if(!downOnly)await page.mouse.up();}
}
try{
 for(const touch of [false,true]){
  const mode=touch?'touch':'desktop';
  const page=await browser.newPage({viewport:{width:touch?390:1100,height:1000},deviceScaleFactor:touch?2:1,isMobile:touch,hasTouch:touch});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto(url,{waitUntil:'domcontentloaded'});
  await page.waitForFunction(()=>window.__imguiReviewCatalog?.length===48&&window.__imguiReview);
  const catalog=await page.evaluate(()=>window.__imguiReviewCatalog);manifest.catalog=catalog;
  for(const dark of [0,1])for(const c of catalog){
   for(let s=0;s<states.length;++s){
    if(!(c.states&(1<<s)))continue;
    await configure(page,c.id,s,dark);
    // A second frame settles popup sizing, layout and the font atlas.
    await page.waitForTimeout(35);
    const file=`${mode}-${dark?'dark':'light'}-${String(c.id).padStart(2,'0')}-${c.name}-${states[s]}.png`;
    await page.screenshot({path:`${root}/${file}`,scale:'css'});
    const review=await page.evaluate(()=>window.__imguiReview);
    if(c.id<=13&&[2,3,4].includes(s)){
      const before=review.changes;
      assert(review.rect.w>0&&review.rect.h>0,`${c.name} must have real input bounds`);
      await pointer(page,review.rect,touch);
      await page.evaluate(()=>Module.ccall('mobile_insert_utf8',null,['string'],['Z']));
      await page.waitForTimeout(60);
      assert.equal((await page.evaluate(()=>window.__imguiReview)).changes,before,`${mode}/${c.name}/${states[s]} must reject mutation`);
      manifest.interactions.push(`${mode}/${c.name}/${states[s]}:locked`);
    }
    manifest.renders.push({component:c.name,state:states[s],mode,dark:!!dark,file});
   }
  }
  // Real pointer transitions, plus keyboard focus. Never force a visual hover flag.
  await configure(page,0,0,0);let r=await page.evaluate(()=>window.__imguiReview.rect);
  if(!touch){
    await page.mouse.move(r.x+20,r.y+r.h/2);
    await page.waitForFunction(()=>window.__imguiReview.hover);
    await page.screenshot({path:`${root}/button-real-hover.png`,scale:'css'});
    await page.mouse.down();await page.waitForFunction(()=>window.__imguiReview.active);
    await page.screenshot({path:`${root}/button-real-pressed.png`,scale:'css'});await page.mouse.up();
    // Native Tab traversal must eventually focus the preview action, without test-set NavIDs.
    let focused=false;
    for(let i=0;i<18&&!focused;++i){await page.keyboard.press('Tab');await page.waitForTimeout(35);focused=await page.evaluate(()=>window.__imguiReview.focused);}
    assert(focused,'Tab must reach the action');
    await page.screenshot({path:`${root}/button-real-focus.png`,scale:'css'});
    let before=await page.evaluate(()=>window.__imguiReview.changes);await page.keyboard.press('Space');
    await page.waitForFunction(before=>window.__imguiReview.changes>before,before);
  }else{
    await pointer(page,r,true);await page.waitForFunction(()=>window.__imguiReview.changes===1);
    assert(!(await page.evaluate(()=>window.__imguiReview.hover)),'Touch release must not leave mouse hover');
  }
  if(!touch)for(const c of catalog.filter(c=>c.interactive)){
    await configure(page,c.id,0,0);await page.waitForTimeout(35);
    let info=await page.evaluate(()=>window.__imguiReview);
    if(info.rect.w<=0||info.rect.h<=0||info.disabled){
      manifest.interactions.push(`${c.name}: no enabled primary target (composite boundary)`);continue;
    }
    await page.mouse.move(info.rect.x+Math.min(20,info.rect.w/2),info.rect.y+info.rect.h/2);
    await page.waitForFunction(()=>window.__imguiReview.hover).catch(()=>{throw Error(`${c.name}: real hover was not reported`);});
    await page.waitForTimeout(190);
    await page.screenshot({path:`${root}/desktop-${c.name}-real-hover.png`,scale:'css'});
    if(c.id<=13){
      await page.mouse.down();await page.waitForTimeout(70);
      await page.screenshot({path:`${root}/desktop-${c.name}-real-active.png`,scale:'css'});await page.mouse.up();
    }
    await configure(page,c.id,0,0);await page.waitForTimeout(35);
    let focused=false;
    for(let attempt=0;attempt<24&&!focused;++attempt){await page.keyboard.press('Tab');await page.waitForTimeout(35);focused=await page.evaluate(()=>window.__imguiReview.focused);}
    assert(focused,`${c.name}: keyboard navigation must reach the primary control`);
    await page.screenshot({path:`${root}/desktop-${c.name}-real-focus.png`,scale:'css'});
    manifest.interactions.push(`${c.name}: real hover and keyboard focus`);
  }
  // Toggle labels, not just knobs, are actionable.
  for(const c of [4,5,6]){await configure(page,c,0,0);r=await page.evaluate(()=>window.__imguiReview.rect);
    await pointer(page,{...r,x:r.x+r.w-12,w:10},touch);await page.waitForFunction(()=>window.__imguiReview.changes>0);
  }
  // All button variants and five sizes on the same real rendering path.
  for(let variant=0;variant<10;++variant){await configure(page,0,0,0);await page.evaluate(v=>Module.ccall('gallery_review_variant',null,['number'],[v]),variant);await page.waitForTimeout(35);
    await page.screenshot({path:`${root}/${mode}-button-variant-${variant}.png`,scale:'css'});}
  for(let size=0;size<5;++size){await configure(page,4,0,0,size);await page.screenshot({path:`${root}/${mode}-checkbox-size-${size}.png`,scale:'css'});}
  assert.deepEqual(errors,[],`${mode} runtime errors`);
  console.log(mode,'all component states, locking and real input passed');await page.close();
 }
 fs.writeFileSync(`${root}/manifest.json`,JSON.stringify(manifest,null,2));
 console.log('state review:',manifest.renders.length,'renders;',manifest.interactions.length,'locking checks');
}finally{await browser.close();}
