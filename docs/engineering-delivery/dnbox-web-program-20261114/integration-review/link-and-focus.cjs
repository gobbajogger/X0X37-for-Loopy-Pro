// Focused read-only user checks, with normal browser TLS verification.
const fs=require('fs');const path=require('path');const {chromium}=require('playwright');const OUT=__dirname;const BASE='http://127.0.0.1:4323/';
(async()=>{const browser=await chromium.launch({headless:true,executablePath:'/usr/bin/chromium'});const checks=[];
try{
for(const route of ['files','learn/versions','companion']){
 const page=await browser.newPage({viewport:{width:390,height:844}});await page.goto(BASE+'#'+route);await page.locator('h1').waitFor();
 const initial=await page.evaluate(()=>({tag:document.activeElement.tagName,y:scrollY,hash:location.hash}));await page.keyboard.press('Tab');
 checks.push({case:'fresh-direct-hash',route,initial,firstTab:await page.evaluate(()=>({tag:document.activeElement.tagName,href:document.activeElement.getAttribute('href'),text:document.activeElement.innerText}))});await page.close();
}
const page=await browser.newPage({viewport:{width:390,height:844}});await page.goto(BASE+'#files');await page.getByText('What fits, and how will it sound?',{exact:true}).click();const target=page.getByRole('link',{name:'Match your firmware & files',exact:false});await target.scrollIntoViewIfNeeded();await target.focus();await page.waitForTimeout(80);const before=await page.evaluate(()=>({y:scrollY,focus:document.activeElement.textContent,open:document.querySelector('details').open}));await page.keyboard.press('Enter');await page.waitForTimeout(80);const navigated=await page.evaluate(()=>({hash:location.hash,y:scrollY,active:document.activeElement.tagName}));await page.goBack();await page.waitForTimeout(80);const back=await page.evaluate(()=>({y:scrollY,focus:document.activeElement.textContent,open:document.querySelector('details').open}));checks.push({case:'keyboard-history-exact',before,navigated,back});await page.close();
for(const [label,url] of [
['keyboard manual','https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/a19da721c25ab2323bb818851ab4d92e8ca3aa15/docs/MANUAL.md'],
['browser manual','https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/a19da721c25ab2323bb818851ab4d92e8ca3aa15/web/MANUAL.md'],
['offline editor source','https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/94197fc839b00aff47b98ca02f3c6396e4a6eabf/web/offline-sequencer'],
['TouchOSC source','https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/035026fe70059af35e25c7c121b40f318b732fe3/web/offline-sequencer/touchosc/README.md'],
['capability contract','https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/e2d36dba34a2fed3bdb192d509687679674ccd7e/experiments/smk37-browser-contract-20261114/CAPABILITIES.md'],
['browser manual keyboard link','https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/a19da721c25ab2323bb818851ab4d92e8ca3aa15/web/legacy.html']]){
 const p=await browser.newPage();try{const response=await p.goto(url,{timeout:20000,waitUntil:'domcontentloaded'});checks.push({case:'external-link',label,url,status:response.status(),title:await p.title(),text:(await p.locator('body').innerText()).slice(0,250)});}catch(e){checks.push({case:'external-link',label,url,error:e.message});}finally{await p.close();}
}
}finally{await browser.close();fs.writeFileSync(path.join(OUT,'link-and-focus-results.json'),JSON.stringify(checks,null,2));}console.log(JSON.stringify(checks,null,2));})().catch(e=>{console.error(e);process.exitCode=1});
