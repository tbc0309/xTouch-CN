// Browser-only view preferences. Never store configuration or credentials here.
function createPortalViewState(env=window,doc=document){
 const key='xtouch.view.v1';let state={tab:'devices',panels:{},positions:{}},ready=false,pending=null,timer=0,restoreTimer=0;
 try{const saved=JSON.parse(env.localStorage.getItem(key)||'null');if(saved&&['devices','cloudAccounts','settings','maintenance'].includes(saved.tab))state={tab:saved.tab,panels:saved.panels&&typeof saved.panels==='object'&&!Array.isArray(saved.panels)?saved.panels:{},positions:saved.positions&&typeof saved.positions==='object'&&!Array.isArray(saved.positions)?saved.positions:{}};}catch{}
 const write=()=>{try{env.localStorage.setItem(key,JSON.stringify(state));}catch{}};
 function capture(){
  if(!ready||pending||doc.getElementById('app')?.hidden)return;
  const tab=doc.getElementById(state.tab);let closest=null,distance=Infinity;
  for(const el of tab?.querySelectorAll('[id]')||[]){const r=el.getBoundingClientRect();if(r.width>0&&r.height>0&&r.bottom>0&&Math.abs(r.top)<distance){closest=el;distance=Math.abs(r.top);}}
  state.positions[state.tab]={y:Math.max(0,env.scrollY||0),anchor:closest?.id||'',offset:closest?closest.getBoundingClientRect().top:0};write();
 }
 function layoutChanged(){
  if(!pending)return;
  env.requestAnimationFrame(()=>{
   if(!pending)return;
   const anchor=pending.anchor&&doc.getElementById(pending.anchor);
   if(pending.anchor&&(!anchor||!anchor.getBoundingClientRect().height))return;
   const y=anchor?env.scrollY+anchor.getBoundingClientRect().top-pending.offset:pending.y;
   pending=null;env.scrollTo({top:Math.max(0,y||0),behavior:'instant'});
  });
 }
 env.addEventListener('scroll',()=>{if(pending)return;env.clearTimeout(timer);timer=env.setTimeout(capture,120);},{passive:true});
 env.addEventListener('pagehide',capture);
 for(const event of ['wheel','touchstart','keydown','pointerdown'])env.addEventListener(event,()=>{pending=null;},{passive:true});
 return {
  capture,layoutChanged,
  tab:()=>state.tab,
  select(id){if(!['devices','cloudAccounts','settings','maintenance'].includes(id))return;state.tab=id;ready=true;pending=state.positions[id]||{y:0};write();layoutChanged();env.clearTimeout(restoreTimer);restoreTimer=env.setTimeout(()=>{if(pending){const y=pending.y;pending=null;env.scrollTo({top:Math.max(0,y||0),behavior:'instant'});}},1200);},
  expanded(id){return state.panels[id]===true;},
  panel(id,open){state.panels[id]=!!open;write();layoutChanged();},
 };
}
