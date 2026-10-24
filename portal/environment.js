// Optional Xiaomi BLE thermometer panel. Credentials are saved independently.
let environmentCard=null,environmentKnown=null,environmentBusy=false;
function environmentProductLabel(value){
 const names={KS2BB:t("领普温湿度传感器","Linptech Temperature and Humidity Sensor"),LYWSD03MMC:t("小米米家蓝牙温湿度计 2","Xiaomi Mijia Bluetooth Thermometer 2"),MJWSD05MMC:t("米家智能温湿度计 3","Mijia Smart Thermometer 3"),MJWSD06MMC:t("米家智能温湿度计 3 mini","Mijia Smart Thermometer 3 mini"),LYWSD02MMC:t("小米米家电子温湿度计 Pro","Xiaomi Mijia Digital Thermometer Pro"),XMWSDJ04MMC:t("小米电子温湿度计","Xiaomi Digital Thermometer"),LYWSDCGQ:t("米家蓝牙温湿度计","Mijia Bluetooth Thermometer"),LYWSD02:t("小米米家电子温湿度计 Pro","Xiaomi Mijia Digital Thermometer Pro"),CGG1:t("青萍蓝牙温湿度计","Qingping Bluetooth Thermometer"),CGDK2:t("青萍蓝牙温湿度计 Lite","Qingping Bluetooth Thermometer Lite"),CGDK3:t("青萍温湿度计 Lite","Qingping Thermometer Lite"),CGD1:t("青萍蓝牙闹钟","Qingping Bluetooth Alarm Clock"),"MHO-C303":t("秒秒测时钟温湿度计","MiaoMiaoce Thermometer Clock"),"MHO-C401":t("秒秒测蓝牙温湿度计","MiaoMiaoce Bluetooth Thermometer"),CGC1:t("青萍蓝牙小钟","Qingping Bluetooth Clock"),MBS17:t("目博士温湿度计 MBS-17","Miaoboshi Thermometer MBS-17"),ESM787:t("炎米智能温湿度计","Yanmi Smart Thermometer"),CGG3:t("青萍温湿度计","Qingping Thermometer"),MJWSD07MMC:t("米家智能温湿度计 3 Pro","Mijia Smart Thermometer 3 Pro")};
 const name=names[value.model]||value.manufacturer||'Xiaomi BLE';
 return ['zh','zh-TW','ja'].includes(lang)?`${name}（${value.model}）`:`${name} (${value.model})`;
}
function environmentDraft(){return !!environmentCard&&(fieldDirty($('environmentMac'))||!!$('environmentKey').value||(['environmentMac','environmentKey'].some(id=>$(id).dataset.locked==='0')&&environmentKnown?.configured));}
function environmentFields(value){
 environmentKnown=value;$('environmentMac').value=value.mac||'';$('environmentKey').value='';if($('environmentKey').dataset.locked==='0'&&$('environmentKey').onchange)$('environmentKey').onchange();$('environmentDisplay').value=value.display||'alternate';$('environmentInterval').value=value.interval||5;$('environmentEncrypt').checked=!!value.encrypted;
 for(const name of ['environmentMac','environmentKey']){const input=$(name);trackField(input,input.value);if(value.configured&&!input.dataset.environmentEditor){lockEditor(input,environmentButtons);input.dataset.environmentEditor='1';}input.dispatchEvent(new Event('saved'));if(!value.configured){input.readOnly=false;input.dataset.locked='0';input.parentElement.classList.remove('editLocked');input.setAttribute('aria-readonly','false');}}
}
function updateEnvironmentPanel(value,failures=0){
 if(!value)return;
 if(!environmentCard){
  const section=document.createElement('section');section.id='environmentSensor';
  section.innerHTML='<div class="sectionHead"><h2 id="environmentTitle"></h2><div class="sectionTools"><span id="environmentStatus" class="badge" role="status" aria-live="polite"></span><button id="environmentClear" class="danger"></button></div></div><div class="grid"><label><span>MAC</span><input data-environment id="environmentMac" maxlength="17" required pattern="[0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5}" autocomplete="off" spellcheck="false"></label><div class="passwordField"><span>BLE Key</span><label class="checkbox"><input data-environment id="environmentEncrypt" type="checkbox"><span id="environmentEncryptText"></span></label><input data-environment id="environmentKey" type="password" maxlength="32" pattern="[0-9A-Fa-f]{32}" autocomplete="new-password" spellcheck="false"></div><label><span id="environmentDisplayText"></span><select data-environment id="environmentDisplay"><option value="off"></option><option value="alternate"></option><option value="sensor"></option></select></label><label><span class="settingValue"><span id="environmentIntervalText"></span><output id="environmentIntervalValue" for="environmentInterval">5</output></span><input data-environment class="settingRange" id="environmentInterval" type="range" min="1" max="30" step="1" value="5"></label></div><p class="muted"><span id="environmentAge"></span><span id="environmentModel" class="danger"></span></p><p id="environmentHint" class="muted"></p><div class="actions formActions"><button id="environmentSave" class="primary"></button></div>';
  $('settings').append(section);environmentCard=section;fieldHeadingObserver.observe($('environmentKey').parentElement);
  for(const name of ['environmentMac','environmentKey']){trackField($(name),$(name).value);for(const ev of ['input','locked'])$(name).addEventListener(ev,environmentButtons);}
  $('environmentSave').onclick=()=>environmentSave(false);
  $('environmentClear').onclick=async()=>{if(confirm(t('清除绑定','Clear binding')+'?'))await environmentSave(true);};
  for(const name of ['environmentDisplay','environmentInterval'])$(name).onchange=()=>{environmentButtons();if(environmentKnown?.configured&&!environmentDraft()&&$('environmentInterval').checkValidity())environmentSave(false);};
  $('environmentEncrypt').onchange=environmentButtons;$('environmentInterval').oninput=environmentButtons;
 }
 const signature=v=>JSON.stringify([v.configured,v.mac,v.keySaved,v.encrypted,v.display,v.interval]);
 if(!environmentKnown||(!environmentBusy&&!environmentDraft()&&signature(value)!==signature(environmentKnown)))environmentFields(value);
 $('environmentTitle').textContent=t('温湿度计（Xiaomi BLE）','Thermometer (Xiaomi BLE)');
 $('environmentDisplayText').textContent=t('首页显示','Home display');
 const select=$('environmentDisplay');select.options[0].textContent=t('关闭显示','Hide');select.options[1].textContent=t('交替显示','Alternate');select.options[2].textContent=t('仅温湿度','Sensor only');
 $('environmentIntervalText').textContent=t('交替间隔（秒）','Alternating interval (s)');
 $('environmentEncryptText').textContent=t('加密并绑定','Encrypt & bind');
 $('environmentEncrypt').parentElement.title=t('勾选后加密保存，并绑定当前设备密钥；重置或更换设备后需重新填写。未勾选则明文保存。','Encrypts this credential and binds it to this device key. Re-enter it after a reset or on another device. Unchecked: stored as plaintext.');
 $('environmentHint').textContent=t('更换 MAC 时请填写新设备的 BLE Key；留空会清除旧密钥。支持 MiBeacon 原厂广播；其它兼容型号见文档。无 AMS 时交替模式直接显示温湿度；~ 表示旧读数。','When changing MAC, enter the new device BLE Key; leaving it blank clears the old key. Supports stock MiBeacon broadcasts; see documentation for compatible models. Without AMS, Alternate shows sensor readings directly. ~ marks older readings.');
 $('environmentAge').textContent=value.available?`${t('上次上报','Last report')}: T ${Math.round(value.temperatureAgeMs/1000)}s · RH ${Math.round(value.humidityAgeMs/1000)}s`:t('数值未变化时可能不重新上报；等待首次温湿度广播。','Unchanged values may not be reported again; waiting for the first temperature and humidity broadcasts.');
 $('environmentModel').textContent=value.configured&&value.model?' · '+environmentProductLabel(value):'';
 $('environmentSave').textContent=t('保存绑定','Save binding');$('environmentClear').textContent=t('清除绑定','Clear binding');
 $('environmentKey').placeholder=value.keySaved?t('已保存；仅同一 MAC 留空保留','Saved; blank keeps key only for the same MAC'):t('仅加密广播需要','For encrypted broadcasts');
 const status=$('environmentStatus');status.classList.toggle('ok',value.fresh);
 status.textContent=value.available?`${value.fresh?'':'~ '}${value.temperature.toFixed(1)}°C · ${value.humidity.toFixed(0)}%`:!value.configured?t('尚未绑定','Not bound'):value.keyUnavailable?t('密钥不可用，请重新绑定','Key unavailable; bind again'):value.keyRequired?t('需要 BLE Key','BLE Key required'):failures?t('密钥校验失败','Key authentication failed'):t('等待传感器数据','Waiting for sensor data');
 environmentButtons();
}
function environmentButtons(){
 if(!environmentCard)return;
 const mac=$('environmentMac'),key=$('environmentKey'),known=environmentKnown,display=$('environmentDisplay'),seconds=$('environmentInterval'),protect=$('environmentEncrypt');
 const sameMac=!!known?.mac&&mac.value.trim().toUpperCase()===known.mac.toUpperCase();
 const editingEmptyKey=sameMac&&key.dataset.locked==='0'&&!key.value&&known?.keySaved;
 const editing=mac.dataset.locked==='0'||key.dataset.locked==='0';
 const changed=!known?.configured||!sameMac||!!key.value||display.value!==known.display||Number(seconds.value)!==known.interval;
 protect.disabled=environmentBusy||!key.value;if(!key.value)protect.checked=sameMac&&!!known?.encrypted;
 seconds.disabled=display.value!=='alternate';
 $('environmentIntervalValue').textContent=seconds.value;
 const interval=Number(seconds.value),validInterval=Number.isInteger(interval)&&interval>=1&&interval<=30;
 const valid=!editingEmptyKey&&mac.checkValidity()&&!!mac.value&&(display.value!=='alternate'||validInterval)&&(!key.value||/^[\da-f]{32}$/i.test(key.value));
 $('environmentSave').disabled=environmentBusy||!valid||!changed;
 $('environmentSave').hidden=!!known?.configured&&!editing&&!changed;
 $('environmentSave').parentElement.hidden=$('environmentSave').hidden;
 $('environmentClear').hidden=!known?.configured;$('environmentClear').disabled=environmentBusy||!known?.configured;
 for(const input of [mac,key])input.classList.toggle('unsaved',fieldDirty(input));
 display.classList.toggle('unsaved',!!known&&display.value!==known.display);seconds.classList.toggle('unsaved',!!known&&interval!==known.interval);
}
async function environmentSave(clear){
 if(environmentBusy)return;environmentBusy=true;environmentButtons();
 try{
  const seconds=$('environmentInterval');await api('/api/environment',clear?{clear:true}:{mac:$('environmentMac').value.trim(),bindkey:$('environmentKey').value,encrypt:$('environmentEncrypt').checked,display:$('environmentDisplay').value,interval:Number.isInteger(Number(seconds.value))&&Number(seconds.value)>=1&&Number(seconds.value)<=30?Number(seconds.value):(environmentKnown?.interval||5)});
  const saved=await api('/api/environment');environmentFields(saved);updateEnvironmentPanel(saved);
 }catch(error){$('environmentStatus').textContent=failure(error);}
 finally{environmentBusy=false;environmentButtons();}
}
