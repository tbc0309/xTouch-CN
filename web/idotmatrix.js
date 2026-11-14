import {websiteLanguage,saveWebsiteLanguage} from './language.js';

const project='idotmatrix-bambulab';
const boards={
  'lckfb-esp32s3r8n8':{chip:'ESP32-S3',target:'esp32s3',manifest:'manifest.json',ota:0x89000,limit:0x377000,memory:'8 MB Flash · 8 MB PSRAM'},
  'clockwise-esp32-hub75':{chip:'ESP32',target:'esp32',manifest:'clockwise-esp32-hub75.json',boot:0x1000,ota:0x19000,limit:0x1e7000,memory:'4 MB Flash · 520 KB SRAM',screen:'64 × 64 HUB75'},
  'esp32c3-oled-042':{chip:'ESP32-C3',target:'esp32c3',manifest:'esp32c3-oled-042.json',ota:0x19000,limit:0x1e7000,memory:'4 MB Flash · 400 KB SRAM',screen:'72 × 40 OLED'},
  'esp32c3-rgb24':{chip:'ESP32-C3',target:'esp32c3',manifest:'esp32c3-rgb24.json',ota:0x19000,limit:0x1e7000,memory:'4 MB Flash · 400 KB SRAM',screen:'24 × RGB LED'},
  'seeed-xiao-esp32c6':{chip:'ESP32-C6',target:'esp32c6',manifest:'seeed-xiao-esp32c6.json',ota:0x19000,limit:0x1e7000,memory:'4 MB Flash · 512 KB SRAM'},
};
export function validatePixelCatalog(value){
  if(value?.project!==project||typeof value.published!=='boolean'||!Array.isArray(value.boards))throw Error('Invalid pixel project');
  if(!value.published)return null;
  if(!/^\d+\.\d+\.\d+$/.test(value.version)||!value.boards.length||
     new Set(value.boards.map(b=>b.id)).size!==value.boards.length||
     value.releaseUrl!==`https://github.com/tbc0309/${project}/releases/tag/${value.version}`)throw Error('Invalid pixel release');
  for(const b of value.boards){
    const spec=Object.hasOwn(boards,b.id)?boards[b.id]:null;
    if(!spec||b.chip!==spec.chip||b.manifest!=='idotmatrix/'+spec.manifest||!Array.isArray(b.parts)||b.parts.length!==2)throw Error('Invalid pixel board');
    const prefix=`idotmatrix/firmware/${project}-${value.version}-${b.id}-`;
    const boot=spec.boot??0;
    for(const [i,part] of b.parts.entries()){
      if(part.offset!==[boot,spec.ota][i]||part.path!==`${prefix}install-${i}.bin`||!/^[a-f0-9]{64}$/.test(part.sha256)||
         !Number.isInteger(part.size)||part.size<=0||(i===0?part.size!==0x9000-boot:part.size>spec.limit))throw Error('Invalid pixel segments');
    }
  }
  return value.boards[0];
}
export function validatePixelManifest(value,catalog,board){
  const build=value?.builds?.[0];
  const title=board.id==='clockwise-esp32-hub75'?'3D Print Pixel Monitor':'iDotMatrix × Bambu Lab';
  if(value?.name!==title+' · '+board.name||value.version!==catalog.version||value.builds.length!==1||
    build?.chipFamily!==board.chip||build.parts?.length!==2||build.parts.some((p,i)=>p.path!==board.parts[i].path.replace(/^idotmatrix\//,'')||p.offset!==board.parts[i].offset))throw Error('Pixel manifest does not match catalog');
}
const boardPreferenceKey='idotmatrix-bambulab.installer.board';
const words={
  version:['版本','Version'],
  xtouch:['xTouch-CN 刷机','xTouch-CN installer'],eyebrow:['ESP32 · 蓝牙像素屏联动','ESP32 · BLUETOOTH PIXEL DISPLAY'],
  title:['让拓竹打印状态，亮在身边。','Keep your Bambu Lab print status in sight.'],intro:['为 ESP32 开发板安装固件，通过蓝牙将打印状态显示在 iDotMatrix 32×32 像素屏上。Clockwise 直驱 64×64 HUB75，OLED 和 24 灯环板也有板载显示。','Install firmware on an ESP32 board to show printer status on a 32×32 iDotMatrix Bluetooth display. Clockwise drives a 64×64 HUB75 panel directly. OLED and 24-LED ring boards also have onboard displays.'],
  independent:['一款运行于 ESP32 的独立像素屏控制器。','An independent pixel-display controller for ESP32 boards.'],usb:['USB 在线安装','USB WEB INSTALL'],install:['连接并刷写','Connect and install'],board:['选择 ESP32 开发板','Select ESP32 board'],
  step1:['电脑使用 Chrome 或 Edge，通过 USB 连接开发板。','Use desktop Chrome or Edge and connect the board by USB.'],step2:['关闭占用串口的软件，确认主板型号后加载刷写工具。','Close software using the serial port. Check the board model, then load the installer.'],step3:['选择开发板串口，按提示完成安装。','Select the board serial port and follow the installation dialog.'],
  preserve:['保留设置时不要勾选 Erase device；擦除会清除 Wi-Fi、打印机、像素屏绑定和管理密码。安装过程请保持 USB 连接。','Leave Erase device unchecked to preserve settings. Erasing removes Wi-Fi, printers, display binding and the admin password. Keep USB connected while installing.'],
  load:['加载刷写工具','Load installer'],connect:['连接开发板','Connect board'],toolLanguage:['刷写工具的连接、进度和确认窗口由 ESP Web Tools 提供，目前为英文。','Connection, progress and confirmation dialogs are provided by ESP Web Tools in English.'],downloads:['固件下载与发布说明','Firmware downloads and release notes'],
  setupTag:['Wi-Fi 与像素屏','WI-FI & PIXEL DISPLAY'],setup:['首次使用','First setup'],setup1:['连接 PrintPixel-xxxxxx 热点：Clockwise 无密码，其余板型密码为 12345678。','Join PrintPixel-xxxxxx. Clockwise uses an open hotspot; other boards use password 12345678.'],setup2:['打开 http://192.168.4.1/，直接扫描并设置 Wi-Fi，无需登录。','Open http://192.168.4.1/ and configure Wi-Fi directly, without signing in.'],setup3:['连接同一网络，在设备地址首次登录（初始密码 12345678），按提示修改密码，再添加打印机。','On the same network, sign in at the device address (initial password 12345678), change the password, then add a printer.'],setup4:['使用外接蓝牙像素屏时，先断开手机 App，再扫描并启用联动。','For an external Bluetooth display, disconnect its phone app before scanning and enabling the link.'],
  power:['外接蓝牙屏独立供电。Clockwise 通过 HUB75 排线连接灯板，灯板使用 5V 电源。','Power external Bluetooth displays separately. Clockwise uses a HUB75 ribbon cable and a 5 V panel supply.'],boot:['BOOT 长按 3 秒清除 Wi-Fi，长按 10 秒清除全部配置。','Hold BOOT for three seconds to clear Wi-Fi, or ten seconds to clear all settings.'],project:['项目与功能介绍','Project and features'],
  checking:['正在检查固件…','Checking firmware…'],unavailable:['固件待发布，在线安装暂未开放。','Firmware has not been published. Online installation is not yet available.'],invalid:['固件目录暂不可用，请稍后重试。','Firmware catalog unavailable. Please retry later.'],unsupported:['在线安装需要电脑上的 Chrome 或 Edge，以及 HTTPS 或 localhost 环境。','Use desktop Chrome or Edge over HTTPS or localhost to install.'],loading:['正在校验固件并加载刷写工具…','Verifying firmware and loading the installer…'],failed:['固件校验或工具加载失败，请重试。','Firmware verification or installer loading failed. Please retry.'],ready:['固件已校验，可连接开发板。','Firmware verified. You can connect the board.']
};
async function start(){
  const $=id=>document.getElementById(id);let lang=websiteLanguage(),state='checking',catalog,board;
  const t=key=>words[key]?.[lang==='en'?1:0]||key;
  const boardName=b=>lang==='zh'&&b.id==='lckfb-esp32s3r8n8'?b.name.replace(/^LCKFB /,'立创 '):b.name;
  function language(){
    document.documentElement.lang=lang==='en'?'en':'zh-CN';document.title='iDotMatrix × Bambu Lab · '+(lang==='en'?'Online installer':'在线刷机');$('language').value=lang;
    document.querySelectorAll('[data-t]').forEach(el=>el.textContent=t(el.dataset.t));
    for(const option of $('board').options){
      const entry=catalog?.boards.find(b=>b.id===option.value);
      if(entry)option.textContent=boardName(entry);
    }
    $('installer-status').textContent=t(state);
    const spec=board&&boards[board.id];
    $('board-name').textContent=board?boardName(board):'—';
    $('board-screen').textContent=spec?[spec.chip,spec.screen].filter(Boolean).join(' · '):'—';
    $('version').textContent=board?`${t('version')} v${catalog.version}`:'—';
    const memory=document.querySelector('.board-memory');
    memory.hidden=!spec;
    memory.textContent=spec?.memory||'';
    $('board-specs').hidden=!spec;
    $('board-specs').textContent=memory.textContent;
    const connect=$('connect-installer');if(connect)connect.textContent=t('connect');
    document.querySelectorAll('a[href^="index.html"]').forEach(a=>a.href='index.html?lang='+lang);
  }
  $('language').addEventListener('change',()=>{lang=$('language').value;saveWebsiteLanguage(lang);language();});language();
  try{
    const r=await fetch('idotmatrix-firmware.json',{cache:'no-store'});if(!r.ok)throw Error('Catalog unavailable');catalog=await r.json();board=validatePixelCatalog(catalog);
    if(!board){state='unavailable';language();return;}
    try{board=catalog.boards.find(b=>b.id===localStorage.getItem(boardPreferenceKey))||board;}catch{}
    $('board').replaceChildren(...catalog.boards.map(b=>new Option(boardName(b),b.id)));
    $('board').value=board.id;
    $('board').disabled=false;
    $('board').addEventListener('change',()=>{
      board=catalog.boards.find(b=>b.id===$('board').value);
      if(board)try{localStorage.setItem(boardPreferenceKey,board.id);}catch{}
      $('installer-widget').replaceChildren();
      $('load-installer').hidden=false;
      $('load-installer').disabled=!(window.isSecureContext&&'serial' in navigator);
      state=$('load-installer').disabled?'unsupported':'ready';
      language();
    });
    $('release-link').href=catalog.releaseUrl;$('release-link').hidden=false;
    state=window.isSecureContext&&'serial' in navigator?'ready':'unsupported';$('load-installer').disabled=state!=='ready';
  }catch{state='invalid';}language();
  $('load-installer').addEventListener('click',async()=>{
    if(!board||$('load-installer').disabled)return;$('load-installer').disabled=true;$('board').disabled=true;state='loading';language();let timer;
    try{
      await Promise.race([(async()=>{
        const r=await fetch(board.manifest,{cache:'no-store'});if(!r.ok)throw Error('Missing manifest');const manifest=await r.json();validatePixelManifest(manifest,catalog,board);
        for(const part of board.parts){const data=await fetch(part.path,{cache:'no-store'});if(!data.ok)throw Error('Missing image');const raw=await data.arrayBuffer();if(raw.byteLength!==part.size)throw Error('Image size mismatch');const digest=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',raw)),x=>x.toString(16).padStart(2,'0')).join('');if(digest!==part.sha256)throw Error('Image checksum mismatch');}
        await import('./vendor/install-button.js');await customElements.whenDefined('esp-web-install-button');
      })(),new Promise((_,reject)=>{timer=setTimeout(()=>reject(Error('Installer timeout')),25000);})]);
      const installer=document.createElement('esp-web-install-button');installer.setAttribute('manifest',board.manifest);const connect=document.createElement('button');connect.id='connect-installer';connect.slot='activate';connect.className='button primary';connect.textContent=t('connect');installer.append(connect);$('installer-widget').replaceChildren(installer);$('load-installer').hidden=true;state='ready';
    }catch{state='failed';$('load-installer').disabled=false;}finally{clearTimeout(timer);$('board').disabled=false;language();}
  });
}
if(typeof document!=='undefined')start();
