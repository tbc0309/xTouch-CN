import {languageMetadata} from './language.js';
// Configuration is assembled locally. Credentials are never sent by this page.
import timezones from './timezones.js'; // Generated from the shared time-zone database by build_web.py.
const words = {
  title: ['配置文件生成器（备用）', 'Configuration generator (alternative)'],
  eyebrow: ['配置 · 备份', 'CONFIGURE · BACK UP'],
  intro: ['填写后下载配置，在设备后台导入，或解压配置包至 TF 卡。', 'Download a configuration to import in the device portal, or extract the SD package onto your card.'],
  privacy: ['仅在当前浏览器生成，不上传、不记住密码。下载文件包含明文凭据，请妥善保管。', 'Generated in this browser only. Passwords are neither uploaded nor remembered. Downloaded files contain plaintext credentials; keep them private.'],
  wifi: ['Wi-Fi', 'Wi-Fi'], ssid: ['Wi-Fi 名称', 'Wi-Fi name'], password: ['Wi-Fi 密码（开放网络留空）', 'Wi-Fi password (leave empty for an open network)'],
  printers: ['局域网打印机', 'LAN printers'], addPrinter: ['添加局域网设备', 'Add LAN printer'],
  printerHint: ['可选；型号、序列号、地址和访问码须与打印机一致。', 'Optional. Model, serial number, address and access code must match the printer.'],
  name: ['名称 / 备注', 'Name / label'], model: ['设备型号', 'Printer model'], serial: ['序列号', 'Serial number'], host: ['IP / 主机名', 'IP / hostname'], code: ['访问码', 'Access code'], remove: ['删除', 'Remove'],
  accounts: ['拓竹账号 · Access Token', 'Bambu accounts · Access Token'], addAccount: ['添加拓竹账号', 'Add Bambu account'],
  accountHint: ['粘贴已有 Token；此页面不验证 Token、不获取设备。导入后在设备后台验证并关联打印机。', 'Paste an existing Token. This page does not validate Tokens or fetch devices. Verify and link printers in the device portal after importing.'],
  region: ['区域', 'Region'], china: ['中国区', 'China'], global: ['国际区', 'Global'], token: ['Access Token', 'Access Token'], guide: ['获取教程', 'Guide'],
  preferences: ['显示与时间', 'Display and time'], language: ['屏幕语言', 'Screen language'], portalLanguage: ['后台语言', 'Portal language'], brightness: ['亮度（0–255）', 'Brightness (0–255)'], volume: ['音量（0–100%）', 'Volume (0–100%)'], standby: ['联网待机（分钟，0 为关闭）', 'Standby (minutes; 0 disables)'],
  targetBoard: ['目标主板', 'Controller board'], boardHint: ['请选择实际主板。标准 CYD 固件仅提供简体中文和英语，最多20台设备、10个账号。', 'Select your controller. Standard CYD firmware provides Simplified Chinese and English only, with up to 20 devices and 10 accounts.'],
  es3Board: ['LCDWIKI ES3C35P', 'LCDWIKI ES3C35P'],
  ntp: ['NTP 服务器', 'NTP server'], custom: ['自定义', 'Custom'], ntpCustom: ['自定义 NTP 地址', 'Custom NTP address'], zone: ['时区', 'Time zone'], autoZone: ['首次访问设备后台时自动同步', 'Set automatically on first device portal visit'],
  export: ['生成与下载', 'Generate and download'], download: ['下载后台导入配置', 'Download portal configuration'], sd: ['下载 TF 卡配置包', 'Download SD configuration package'], preview: ['预览结构（凭据已隐藏）', 'Preview structure (credentials hidden)'],
  exported: ['已生成配置，请在设备上验证连接。', 'Configuration generated. Verify connections on the device.'], invalid: ['请检查标记的输入项。名称仅限英文、数字和符号。', 'Check the marked fields. Names accept ASCII letters, numbers and symbols.'], byteLimit: ['超过允许的 UTF-8 字节数。', 'Exceeds the allowed UTF-8 byte length.'], duplicate: ['存在重复的序列号和地址。', 'Duplicate serial number and address.'], limit: ['已达到数量上限。', 'The item limit has been reached.'],
};
const models = {'C12':'P1S','C11':'P1P','BL-P001':'X1C','BL-P002':'X1','C13':'X1E','N2S':'A1','N1':'A1 mini','N7':'P2S','N6':'X2D','N9':'A2L','O1D':'H2D','O1E':'H2D Pro','O1S':'H2S','O1C':'H2C'};
const encoder = new TextEncoder();
function node(tag, props = {}, ...children) {
  const el = document.createElement(tag);
  for (const [key,value] of Object.entries(props)) {
    if (key === 'text') el.dataset.configT = value;
    else if (key === 'class') el.className = value;
    else el.setAttribute(key,value);
  }
  el.append(...children); return el;
}
function field(key, control) { return node('label',{},node('span',{text:key}),control); }
function input(name, props = {}) { return node('input',{name,...props}); }
function select(name, options) {
  return node('select',{name},...Object.entries(options).map(([value,label]) => words[label] ? node('option',{value,text:label}) : node('option',{value},label)));
}
// A small uncompressed ZIP keeps SD paths explicit without a runtime dependency.
export function sdZip(files) {
  const chunks=[],directory=[]; let offset=0;
  const crc = bytes => { let value=0xffffffff;for(const b of bytes){value^=b;for(let i=0;i<8;i++)value=(value>>>1)^((value&1)?0xedb88320:0);}return (value^0xffffffff)>>>0; };
  for(const [name,text] of Object.entries(files)) {
    const path=encoder.encode(name),bytes=encoder.encode(text),checksum=crc(bytes);
    const local=new Uint8Array(30+path.length),l=new DataView(local.buffer);
    l.setUint32(0,0x04034b50,true);l.setUint16(4,20,true);l.setUint16(12,33,true);l.setUint32(14,checksum,true);l.setUint32(18,bytes.length,true);l.setUint32(22,bytes.length,true);l.setUint16(26,path.length,true);local.set(path,30);
    const central=new Uint8Array(46+path.length),c=new DataView(central.buffer);
    c.setUint32(0,0x02014b50,true);c.setUint16(4,20,true);c.setUint16(6,20,true);c.setUint16(14,33,true);c.setUint32(16,checksum,true);c.setUint32(20,bytes.length,true);c.setUint32(24,bytes.length,true);c.setUint16(28,path.length,true);c.setUint32(42,offset,true);central.set(path,46);
    chunks.push(local,bytes);directory.push(central);offset+=local.length+bytes.length;
  }
  const size=directory.reduce((sum,b)=>sum+b.length,0),end=new Uint8Array(22),e=new DataView(end.buffer);
  e.setUint32(0,0x06054b50,true);e.setUint16(8,directory.length,true);e.setUint16(10,directory.length,true);e.setUint32(12,size,true);e.setUint32(16,offset,true);
  return new Blob([...chunks,...directory,end],{type:'application/zip'});
}
export function initConfigGenerator(root, initialLanguage = 'en', headingTag = 'h2', initialBoard = 'cyd') {
  if (!root) return;
  let language=initialLanguage, sequence=0;
  const t=key=>words[key][language==='en'?1:0];
  const form=node('form',{autocomplete:'off'}),printers=node('div'),accounts=node('div');
  const status=node('p',{class:'generator-status',role:'status','aria-live':'polite'});
  const preview=node('pre',{},'{}');
  const ssid=input('ssid',{required:'',maxlength:32}),password=input('pwd',{type:'password',maxlength:64});
  const screenLanguage=select('language',Object.fromEntries(languageMetadata.map(row=>[row.code,row.name])));
  screenLanguage.value='en';
  const portalLanguage=select('portalLanguage',Object.fromEntries(languageMetadata.map(row=>[row.code,row.name])));
  portalLanguage.value='en';
  const board=select('board',{cyd:'CYD · ESP32-2432S028R',s3:'Waveshare S3 V2',p4:'Waveshare P4',es3c35p:'es3Board'});
  board.value=['cyd','s3','p4','es3c35p'].includes(initialBoard)?initialBoard:'cyd';
  function boardLanguages() {
    const available=languageMetadata.filter(row=>board.value!=='cyd'||['zh','en'].includes(row.code));
    for(const control of [screenLanguage,portalLanguage]) {
      const previous=control.value;
      control.replaceChildren(...available.map(row=>node('option',{value:row.code},row.name)));
      control.value=available.some(row=>row.code===previous)?previous:'en';
    }
  }
  boardLanguages();
  const brightness=input('backlight',{type:'number',min:0,max:255,value:200,required:''});
  const volume=input('soundVolume',{type:'number',min:0,max:100,value:20,required:''});
  const standby=input('tftOff',{type:'number',min:0,max:240,value:15,required:''});
  const ntp=select('ntpServer',{'ntp.aliyun.com':'Aliyun · ntp.aliyun.com','time.cloudflare.com':'Cloudflare · time.cloudflare.com','pool.ntp.org':'NTP Pool · pool.ntp.org','custom':'custom'});
  const customNtp=input('customNtp',{maxlength:127,placeholder:'ntp.example.com'}),customField=field('ntpCustom',customNtp);customField.hidden=true;
  const zone=node('select',{name:'timezone'},node('option',{value:'',text:'autoZone'}));
  for(const name of Object.keys(timezones).sort())zone.append(node('option',{value:name},name));
  function card(title, ...content) { return node('section',{class:'card'},node('h3',{text:title}),...content); }
  function heading(title,add) { return node('div',{class:'section-heading compact'},node('h3',{text:title}),add); }
  function button(key,click,kind='secondary') { const b=node('button',{type:'button',class:'button '+kind,text:key});b.addEventListener('click',click);return b; }
  function refreshLanguage() {
    root.querySelectorAll('[data-config-t]').forEach(el=>el.textContent=t(el.dataset.configT));
    const now=new Date();
    const selected=zone.value,options=[...zone.options].filter(option=>option.value);
    for(const option of options) {
      const standard=timezones[option.value].match(/^(?:<[^>]+>|[A-Za-z]{3,})([+-]?\d+)(?::(\d+))?/);
      let minutes=standard?-(Number(standard[1])*60+(standard[1].startsWith('-')?-1:1)*Number(standard[2]||0)):0;
      let localName=option.value;
      try {
        const parts=new Intl.DateTimeFormat(language==='en'?'en':'zh-CN',{timeZone:option.value,timeZoneName:'longGeneric'}).formatToParts(now);
        localName=parts.find(part=>part.type==='timeZoneName')?.value||localName;
        const offset=new Intl.DateTimeFormat('en',{timeZone:option.value,timeZoneName:'longOffset'}).formatToParts(now).find(part=>part.type==='timeZoneName')?.value;
        const match=offset?.match(/^GMT(?:([+-])(\d{1,2}):(\d{2}))?$/);
        if(match)minutes=match[1]?(match[1]==='-'?-1:1)*(Number(match[2])*60+Number(match[3])):0;
      } catch {} // Keep the IANA name available even in browsers with an older zone database.
      const absolute=Math.abs(minutes),offset='UTC'+(minutes<0?'-':'+')+Math.floor(absolute/60)+(absolute%60?':'+String(absolute%60).padStart(2,'0'):'');
      option.textContent=offset+' · '+localName+(localName===option.value?'':'（'+option.value+'）');
      option.dataset.offset=minutes;
    }
    options.sort((a,b)=>Number(a.dataset.offset)-Number(b.dataset.offset)||a.value.localeCompare(b.value));
    zone.append(...options);
    zone.value=selected;
  }
  function removeButton(container) { return button('remove',()=>{container.remove();status.textContent='';updatePreview();}); }
  function profileLimits() { return board.value === "cyd" ? {devices:20, accounts:10} : {devices:100, accounts:50}; }
  function addPrinter() {
    if(printers.children.length>=profileLimits().devices){status.textContent=t('limit');return;}
    const box=node('fieldset',{class:'generator-item'}),name=input('name',{required:'',maxlength:63,pattern:'[ -~]*',value:'Printer '+(printers.children.length+1)});
    const model=select('printerModel',models),serial=input('serialNumber',{required:'',maxlength:31,pattern:String.raw`[A-Za-z0-9_\-]+`});
    const host=input('host',{required:'',maxlength:63,pattern:String.raw`[A-Za-z0-9_.\-]+`,placeholder:'printer.local'}),code=input('accessCode',{required:'',type:'password',minlength:8,maxlength:8,pattern:'[!-~]{8}'});
    box.append(node('legend',{},'LAN'),node('div',{class:'grid'},field('name',name),field('model',model),field('serial',serial),field('host',host),field('code',code)),removeButton(box));
    box.controls={name,model,serial,host,code};printers.append(box);refreshLanguage();name.focus();updatePreview();
  }
  function addAccount() {
    if(accounts.children.length>=profileLimits().accounts){status.textContent=t('limit');return;}
    const box=node('fieldset',{class:'generator-item'}),name=input('name',{maxlength:63,pattern:'[ -~]*',value:'Bambu '+(accounts.children.length+1)}),region=select('region',{'cn':'china','global':'global'});
    const token=input('token',{type:'password',required:'',maxlength:2048,pattern:'[!-~]+'});
    box.accountId='account-'+(++sequence);
    box.append(node('legend',{},'Access Token'),node('div',{class:'grid'},field('name',name),field('region',region)),field('token',token),removeButton(box));
    box.controls={name,region,token};accounts.append(box);refreshLanguage();name.focus();updatePreview();
  }
  const help=node('a',{href:'https://github.com/tbc0309/xTouch-CN/blob/main/docs/INSTALLATION.md#token',target:'_blank',rel:'noopener noreferrer',text:'guide'});
  const controls=node('div',{class:'grid'},field('targetBoard',board),field('language',screenLanguage),field('portalLanguage',portalLanguage),field('brightness',brightness),field('volume',volume),field('standby',standby),field('ntp',ntp),field('zone',zone),customField);
  form.append(card('wifi',node('div',{class:'grid'},field('ssid',ssid),field('password',password))),
    node('section',{class:'card'},heading('printers',button('addPrinter',addPrinter)),node('p',{class:'subtle',text:'printerHint'}),printers),
    node('section',{class:'card'},heading('accounts',button('addAccount',addAccount)),node('p',{class:'subtle',text:'accountHint'}),help,accounts),card('preferences',node('p',{class:'subtle',text:'boardHint'}),controls));
  function configuration() {
    return {ssid:ssid.value,pwd:password.value,activePrinter:0,
      printers:[...printers.children].map(box=>{const c=box.controls;return {name:c.name.value,printerModel:c.model.value,serialNumber:c.serial.value,mode:'lan',host:c.host.value,accessCode:c.code.value,accessCodeEncrypted:false};}),
      cloudAccounts:[...accounts.children].map(box=>{const c=box.controls;return {id:box.accountId,name:c.name.value,region:c.region.value,loginMethod:'token',token:c.token.value,tokenEncrypted:false};}),
      settings:{language:screenLanguage.value,portalLanguage:portalLanguage.value,backlight:Number(brightness.value),soundVolume:Number(volume.value),tftOff:Number(standby.value),touchSound:false,warningSound:false,statusLed:true,ota:true,wop:true,deepSleepAfter:0,timezone:timezones[zone.value]||'',timezoneName:zone.value,browserTimezoneSync:!zone.value,ntpServer:ntp.value==='custom'?customNtp.value:ntp.value}};
  }
  function updatePreview() {
    const masked=configuration();masked.pwd=masked.pwd?'••••••••':'';
    for(const p of masked.printers)p.accessCode=p.accessCode?'••••••••':'';
    for(const a of masked.cloudAccounts)a.token=a.token?'••••••••':'';
    preview.textContent=JSON.stringify(masked,null,2);
  }
  function valid() {
    if(printers.children.length>profileLimits().devices || accounts.children.length>profileLimits().accounts){status.textContent=t("limit");return false;}
    for(const el of form.querySelectorAll('input,select')){el.setCustomValidity('');el.removeAttribute('aria-invalid');}
    for(const [el,limit] of [[ssid,32],[password,64]])if(encoder.encode(el.value).length>limit)el.setCustomValidity(t('byteLimit'));
    if(ntp.value==='custom'&&!/^(?=.{1,127}$)[A-Za-z0-9](?:[A-Za-z0-9.-]*[A-Za-z0-9])?$/.test(customNtp.value))customNtp.setCustomValidity(t('invalid'));
    if(ntp.value==='custom'&&customNtp.value.split('.').some(label=>!label||label.length>63||label.startsWith('-')||label.endsWith('-')))customNtp.setCustomValidity(t('invalid'));
    const seen=new Set();for(const box of printers.children){const c=box.controls,key=c.serial.value+'@'+c.host.value;if(!c.name.value.trim())c.name.setCustomValidity(t('invalid'));if(seen.has(key))c.serial.setCustomValidity(t('duplicate'));seen.add(key);}
    const invalid=[...form.querySelectorAll('input,select')].filter(el=>!el.checkValidity());
    invalid.forEach(el=>el.setAttribute('aria-invalid','true'));
    if(invalid.length){status.textContent=t('invalid');invalid[0].reportValidity();return false;}return true;
  }
  function download(blob,name) {
    const url=URL.createObjectURL(blob),a=node('a',{href:url,download:name});a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);status.textContent=t('exported');
  }
  function exportConfig(sd) {
    if(!valid())return;
    const config=configuration();
    if(sd){const settings=config.settings;delete config.settings;download(sdZip({'xtouch.json':JSON.stringify(config,null,2)+'\n','xtouch/settings.json':JSON.stringify(settings,null,2)+'\n'}),'xtouch-config.zip');}
    else download(new Blob([JSON.stringify(config,null,2)+'\n'],{type:'application/json'}),'xtouch.json');
  }
  form.append(card('export',node('div',{class:'actions'},button('download',()=>exportConfig(false),'primary'),button('sd',()=>exportConfig(true))),status,node('details',{},node('summary',{text:'preview'}),preview)));
  root.append(node('div',{class:'hero'},node('div',{},node('p',{class:'eyebrow',text:'eyebrow'}),node(headingTag,{text:'title'})),node('p',{class:'hero-intro',text:'intro'})),node('p',{class:'notice',text:'privacy'}),form);
  ntp.addEventListener('change',()=>{customField.hidden=ntp.value!=='custom';customNtp.required=!customField.hidden;customNtp.disabled=customField.hidden;customNtp.setCustomValidity('');});customNtp.disabled=true;
  form.addEventListener('input',e=>{e.target.setCustomValidity?.('');e.target.removeAttribute('aria-invalid');status.textContent='';updatePreview();});
  form.addEventListener('change',updatePreview);form.addEventListener('submit',e=>e.preventDefault());
  board.addEventListener('change',()=>{boardLanguages();updatePreview();});
  refreshLanguage();updatePreview();
  return {setLanguage(value){language=value;help.href='https://github.com/tbc0309/xTouch-CN/blob/main/docs/INSTALLATION'+(language==='en'?'.en':'')+'.md#token';refreshLanguage();updatePreview();}};
}
