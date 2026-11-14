// Read-only integrated website review. No MIDI, device or external write operations.
const fs=require('fs');const path=require('path');const crypto=require('crypto');const {chromium}=require('playwright');
const OUT=__dirname;const SITE=path.resolve(OUT,'../site');const BASE='http://127.0.0.1:4323/';
const result={utc:new Date().toISOString(),manifest:crypto.createHash('sha256').update(fs.readFileSync(path.join(SITE,'MANIFEST.sha256'))).digest('hex'),browser:[],flows:[],errors:[],links:[]};
const record=(kind,value)=>{result[kind].push(value);fs.writeFileSync(path.join(OUT,'browser-results.json'),JSON.stringify(result,null,2));};
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'/usr/bin/chromium'});result.browserVersion=browser.version();
try{
for(const width of [1440,390,320]){
 const context=await browser.newContext({viewport:{width,height:900},reducedMotion:'reduce'});const page=await context.newPage();page.on('pageerror',e=>record('errors',{width,message:e.message}));
 for(const route of ['home','files','learn/versions','companion']){
  await page.goto(BASE+'#'+route);await page.locator('h1').waitFor();await page.evaluate(()=>document.fonts.ready);
  const state=await page.evaluate(()=>({hash:location.hash,title:document.title,h1:document.querySelector('h1').textContent,active:document.activeElement.tagName,scrollWidth:document.documentElement.scrollWidth,innerWidth,links:[...document.querySelectorAll('main a')].map(a=>({label:a.innerText.trim(),href:a.href})),text:document.querySelector('main').innerText}));
  fs.writeFileSync(path.join(OUT,`${width}-${route.replaceAll('/','-')}.txt`),state.text);
  await page.screenshot({path:path.join(OUT,`${width}-${route.replaceAll('/','-')}.png`),fullPage:true});
  record('browser',{width,route,h1:state.h1,overflow:state.scrollWidth>state.innerWidth,initialActive:state.active});
  if(width===1440)for(const link of state.links)record('links',{from:route,...link});
 }
 // Actual keyboard/direct-hash initialization, then history from changed file-page disclosure.
 await page.goto(BASE+'#files');await page.locator('h1').waitFor();await page.keyboard.press('Tab');
 record('flows',{width,case:'initial-direct-hash-tab',active:await page.evaluate(()=>({tag:document.activeElement.tagName,text:document.activeElement.innerText,href:document.activeElement.getAttribute('href')}))});
 const summary=page.locator('summary').filter({hasText:'What fits, and how will it sound?'});await summary.click();await summary.focus();
 await page.evaluate(()=>document.querySelector('details').scrollIntoView({block:'center'}));await page.waitForTimeout(70);
 const before=await page.evaluate(()=>({y:scrollY,open:document.querySelector('details').open,focus:document.activeElement.tagName}));
 await page.getByRole('link',{name:'Match your firmware & files',exact:false}).click();await page.waitForTimeout(70);
 const navigated=await page.evaluate(()=>({hash:location.hash,y:scrollY,active:document.activeElement.tagName,text:document.activeElement.textContent}));
 await page.goBack();await page.waitForTimeout(100);
 const back=await page.evaluate(()=>({hash:location.hash,y:scrollY,open:document.querySelector('details').open,focus:document.activeElement.tagName,text:document.activeElement.textContent}));
 await page.goForward();await page.waitForTimeout(100);
 const forward=await page.evaluate(()=>({hash:location.hash,y:scrollY,active:document.activeElement.tagName,text:document.activeElement.textContent}));
 record('flows',{width,case:'files-details-version-back-forward',before,navigated,back,forward});
 // Companion profile disclosure -> exact version guidance -> Back retains explanation.
 await page.goto(BASE+'#companion');await page.locator('h1').waitFor();await page.getByText('Current MIDI control limits',{exact:true}).click();
 await page.getByRole('link',{name:'Version-matched playing & setup guide',exact:false}).click();await page.waitForTimeout(60);await page.goBack();await page.waitForTimeout(80);
 record('flows',{width,case:'companion-limits-history',state:await page.evaluate(()=>({hash:location.hash,detailsOpen:document.querySelector('details').open,focus:document.activeElement.textContent,overflow:document.documentElement.scrollWidth>innerWidth}))});
 await context.close();
}
// Complete local connected musician path. Labels are inspected, no device action.
const context=await browser.newContext({viewport:{width:1440,height:1000}});const page=await context.newPage();page.on('pageerror',e=>record('errors',{flow:'musician-path',message:e.message}));
const visit=async(route)=>{await page.goto(BASE+'#'+route);await page.locator('h1').waitFor();record('flows',{case:'musician-path',route,h1:await page.locator('h1').innerText(),links:await page.locator('main a').evaluateAll(as=>as.map(a=>({label:a.innerText.trim(),href:a.href})))});fs.writeFileSync(path.join(OUT,'workflow-'+route.replaceAll('/','-')+'.txt'),await page.locator('main').innerText());};
for(const route of ['listen','learn','learn/start','learn/next','learn/bass','learn/break','learn/movement','learn/arrangement','learn/save','install','help/recover','setups','try'])await visit(route);
await context.close();
}finally{await browser.close();fs.writeFileSync(path.join(OUT,'browser-results.json'),JSON.stringify(result,null,2));}
console.log(JSON.stringify({manifest:result.manifest,browser:result.browser.length,flows:result.flows.length,errors:result.errors},null,2));})().catch(e=>{console.error(e);process.exitCode=1;});
