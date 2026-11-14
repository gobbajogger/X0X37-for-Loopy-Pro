'use strict';
// Keep each browser-history entry's position, including repeated visits to a route.
history.scrollRestoration='manual';
const routeEntry=()=>history.state?.dnboxRoute;
let cachedFocusElement=null,cachedFocus=null,scrollSnapshotPending=false;
let renderedURL=location.href,restoringRoute=false,renderVersion=0;
function focusLocator(active){
 if(!active||active===document.body||active===document.documentElement)return null;
 if(active.id)return {id:active.id};
 if(active.matches('a[href]'))return {href:active.getAttribute('href'),index:[...document.querySelectorAll('a[href]')].filter(a=>a.getAttribute('href')===active.getAttribute('href')).indexOf(active)};
 // Headings, summaries and native controls without IDs survive a route re-render.
 return {tag:active.tagName,index:[...document.getElementsByTagName(active.tagName)].indexOf(active)};
}
function findFocus(locator){
 if(locator?.id)return document.getElementById(locator.id);
 if(locator?.href)return [...document.querySelectorAll('a[href]')].filter(a=>a.getAttribute('href')===locator.href)[locator.index||0];
 if(locator?.tag)return document.getElementsByTagName(locator.tag)[locator.index||0];
 return null;
}
function rememberRoutePosition(){
 const entry=routeEntry();if(!entry||restoringRoute||renderedURL!==location.href)return;
 const active=document.activeElement;
 if(active!==cachedFocusElement){
  cachedFocusElement=active;
  cachedFocus=focusLocator(active);
 }
 const next={...entry,y:scrollY,focus:cachedFocus,details:[...main.querySelectorAll('details')].map((d,index)=>({id:d.id,index,open:d.open}))};
 if(JSON.stringify(entry)===JSON.stringify(next))return;
 history.replaceState({...history.state,dnboxRoute:next},'');
}
function newRouteEntry(){return {y:0,focus:null};}
if(!routeEntry())history.replaceState({...history.state,dnboxRoute:newRouteEntry()},'');
function restoreRoutePosition(saved,version){
 const details=main.querySelectorAll('details');
 // Expanded content must exist before restoring a control or its scroll position.
 saved?.details?.forEach(item=>{
  const disclosure=item.id?document.getElementById(item.id):details[item.index];
  if(disclosure?.matches('details'))disclosure.open=item.open;
 });
 requestAnimationFrame(()=>{
  if(version!==renderVersion)return;
  const focus=findFocus(saved?.focus);
  if(focus)focus.focus({preventScroll:true});
  else document.activeElement?.blur();
  scrollTo(0,saved?.y||0);restoringRoute=false;
 });
}
function showRoute(mode){
 restoringRoute=true;const version=++renderVersion;
 render();renderedURL=location.href;
 cachedFocusElement=null;cachedFocus=null;
 if(mode==='history'){
  restoreRoutePosition(routeEntry(),version);
 }else{
  if(mode==='navigate'){scrollTo(0,0);main.querySelector('h1')?.focus({preventScroll:true});}
  restoringRoute=false;
  if(mode==='navigate')rememberRoutePosition();
 }
}
document.addEventListener('click',event=>{
 const a=event.target.closest('a[href]');
 if(!a||event.defaultPrevented||event.button!==0||event.metaKey||event.ctrlKey||event.shiftKey||event.altKey||a.target||a.hasAttribute('download'))return;
 const url=new URL(a.href,location.href);
 if(url.origin!==location.origin||url.pathname!==location.pathname||url.search!==location.search||!url.hash||url.hash==='#main')return;
 if(url.href===location.href)return;
 event.preventDefault();rememberRoutePosition();
 history.pushState({...history.state,dnboxRoute:newRouteEntry()},'',url.hash);showRoute('navigate');
});
window.addEventListener('scroll',()=>{
 if(restoringRoute||scrollSnapshotPending)return;
 const version=renderVersion;
 scrollSnapshotPending=true;requestAnimationFrame(()=>{scrollSnapshotPending=false;if(version===renderVersion)rememberRoutePosition();});
},{passive:true});
document.addEventListener('focusin',rememberRoutePosition);
document.addEventListener('toggle',event=>{if(event.target.matches('details'))rememberRoutePosition();},true);
window.addEventListener('pagehide',rememberRoutePosition);
window.addEventListener('pageshow',event=>{
 // A cached document already has its controls; keep them while restoring position.
 if(event.persisted){restoringRoute=true;restoreRoutePosition(routeEntry(),++renderVersion);}
});
window.addEventListener('popstate',()=>{
 // Direct fragment changes emit popstate with no state before hashchange.
 if(routeEntry())showRoute('history');
 else{history.replaceState({...history.state,dnboxRoute:newRouteEntry()},'');showRoute('navigate');}
});
window.addEventListener('hashchange',()=>{
 // A history traversal also emits hashchange; popstate has already rendered it.
 if(renderedURL===location.href)return;
 history.replaceState({...history.state,dnboxRoute:newRouteEntry()},'');showRoute('navigate');
});
// A full-document Back/Forward can reload rather than restore a cached document.
showRoute(performance.getEntriesByType('navigation')[0]?.type==='back_forward'?'history':'initial');
