const {element:$,text:sharedText,request:api,trackField,lockEditor,failure,setupOptionalPanel,fieldHeadingObserver}=window.XTouchPixelPortal;
const t=(zh,en)=>sharedText(zh,en,typeof pixelLocaleWords==='undefined'?undefined:pixelLocaleWords);
let pixelScanBusy=false,pixelScanTimer=0,pixelDevices=[],pixelScanError="";
let pixelCard=null,pixelKnown=null,pixelBusy=false,pixelLive=null,pixelConfirmedMac="";
function pixelDeviceOptions(){
 const selected=$('pixelMac').value.trim().toUpperCase(),options=[];
 if(selected&&!pixelDevices.some(d=>d.mac===selected))options.push(new Option(t('已保存设备','Saved device')+' · '+selected,selected));
 const placeholder=new Option(t('扫描后选择像素屏','Scan and select a pixel screen'),'');placeholder.disabled=true;placeholder.hidden=true;options.push(placeholder);
 for(const d of pixelDevices)options.push(new Option((d.name||t('未命名像素屏','Unnamed pixel screen'))+' · '+d.mac+' · '+d.rssi+' dBm',d.mac));
 options.push(new Option(t('手动输入 MAC','Enter MAC manually'),'__manual__'));
 $('pixelDevice').replaceChildren(...options);$('pixelDevice').value=$('pixelManualRow').hidden?selected:'__manual__';
}
function pixelScanPause(){if(pixelScanTimer)clearTimeout(pixelScanTimer);pixelScanTimer=0;}
async function pixelScanPoll(){
 pixelScanPause();if(!pixelVisible())return;
 try{
  const result=await api('/api/pixel-screen/scan');
  pixelScanBusy=!!result.active;
  pixelScanError='';
  if(!result.active){pixelDevices=result.devices;pixelDeviceOptions();}
  pixelButtons();
  if(result.active&&pixelVisible())pixelScanTimer=setTimeout(pixelScanPoll,1000);
 }catch(error){pixelScanBusy=false;pixelScanError=error.code==='radio_busy'?t('蓝牙正在连接或扫描，请稍后重试。','Bluetooth is connecting or scanning. Please retry shortly.'):failure(error);pixelButtons();}
}
function pixelDraft(){const value={enabled:$('pixelEnabled').checked,mac:$('pixelMac').value.trim().toUpperCase(),interval:2,layout:$('pixelLayout').value,brightness:Number($('pixelBrightness').value),screenOn:$('pixelPower').checked,idleDisplay:$('pixelIdle').checked?'alternate':'clock',rotate180:$('pixelRotate').checked,resetOnConnect:$('pixelReset').checked,clearOnConnect:$('pixelClearMaterials').checked,encrypt:$('pixelEncrypt').checked};if($('pixelPassword').value)value.password=$('pixelPassword').value;if($('pixelClearPassword').checked)value.clearPassword=true;return value;}
function pixelChanged(){return pixelKnown&&JSON.stringify(pixelDraft())!==JSON.stringify({enabled:pixelKnown.enabled,mac:pixelKnown.mac,interval:2,layout:pixelKnown.layout,brightness:pixelKnown.brightness??80,screenOn:pixelKnown.screenOn??true,idleDisplay:pixelKnown.idleDisplay??'clock',rotate180:pixelKnown.rotate180??false,resetOnConnect:pixelKnown.resetOnConnect??false,clearOnConnect:pixelKnown.clearOnConnect??false,encrypt:pixelKnown.encrypted??true});}
function pixelButtons(){
 if(!pixelCard)return;
 for(const id of ['pixelMac','pixelPassword','pixelClearPassword','pixelEnabled','pixelLayout','pixelBrightness','pixelPower','pixelRotate','pixelIdle','pixelReset','pixelClearMaterials'])$(id).disabled=pixelBusy;
 $('pixelScan').textContent=pixelScanBusy?t('正在扫描… ','Scanning… ').trim():pixelScanError?t('扫描失败，请重试','Scan failed; retry'):t('扫描附近像素屏','Scan nearby pixel screens');$('pixelScan').title=pixelScanError;
 $('pixelScan').disabled=pixelBusy||pixelScanBusy;$('pixelDevice').disabled=pixelBusy||pixelScanBusy;
 $('pixelEncrypt').disabled=pixelBusy||!$('pixelPassword').value;
 $('pixelClearPassword').disabled=pixelBusy||!($('pixelPassword').value||pixelKnown?.passwordSaved||$('pixelClearPassword').checked);
 $('pixelPower').disabled=pixelBusy||!pixelKnown?.displayControlAvailable;
 const draft=pixelDraft(),mac=$('pixelMac'),valid=(!draft.mac&&!draft.enabled)||(/^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/.test(draft.mac)&&!['00:00:00:00:00:00','FF:FF:FF:FF:FF:FF'].includes(draft.mac));
 $('pixelSave').disabled=pixelBusy||!valid||!Number.isInteger(draft.interval)||draft.interval<1||draft.interval>5||!!draft.password&&!/^\d{6}$/.test(draft.password)||!!draft.password&&draft.clearPassword||!pixelChanged();
 $('pixelSave').hidden=!pixelChanged()&&mac.dataset.locked!=='0'&&$('pixelPassword').dataset.locked!=='0';
 $('pixelBrightnessValue').textContent=draft.brightness+'%';
 $('pixelLayoutHelp').textContent=draft.layout==='progress'?t('已完成进度彩色向右流动，未完成部分显示耗材色。','Completed progress flows right in rainbow colours; remaining progress uses the filament colour.'):t('分层模型与移动喷头，显示进度、层数、余时和温度。','Layered model and moving printhead, with progress, layers, time and temperatures.');
}
function pixelFields(value){
 pixelKnown=value;$('pixelBrightness').value=value.brightness??80;$('pixelPower').checked=value.screenOn??true;$('pixelRotate').checked=value.rotate180??false;$('pixelIdle').checked=value.idleDisplay==='alternate';$('pixelReset').checked=value.resetOnConnect===true;$('pixelClearMaterials').checked=value.clearOnConnect===true;$('pixelEncrypt').checked=value.encrypted??true;$('pixelPassword').value='';$('pixelClearPassword').checked=false;$('pixelClearRow').hidden=false;$('pixelPassword').placeholder=value.passwordSaved?t('已保存；同一MAC留空保留','Saved; blank keeps it for the same MAC'):t('未设置密码可留空','Leave blank if no device password');$('pixelEnabled').checked=value.enabled;$('pixelMac').value=value.mac||'';$('pixelLayout').value=value.layout;
 for(const name of ['pixelMac','pixelPassword']){
  const input=$(name);if(input.dataset.locked==='0'&&input.onchange)input.onchange();trackField(input,input.value);
  if(!input.dataset.pixelEditor){lockEditor(input,pixelButtons,()=>name==='pixelMac'?!!pixelKnown?.mac:!!pixelKnown?.passwordSaved);input.dataset.pixelEditor='1';}
  input.dispatchEvent(new Event('saved'));
  if(!(name==='pixelMac'?value.mac:value.passwordSaved)){input.readOnly=false;input.dataset.locked='0';input.parentElement.classList.remove('editLocked');input.setAttribute('aria-readonly','false');}
 }
 pixelDeviceOptions();pixelButtons();
}
function pixelVisible(){return !!pixelCard&&!pixelCard.querySelector('.optionalPanelBody')?.hidden&&!document.hidden&&!$('app').hidden&&!$('settings').hidden;}
let pixelAnimation=0,pixelTick=0,pixelAnimationKey='';
function pixelPreviewSnapshot(){
 const selected=$('pixelState').value;
 return selected==='live'?{...pixelLive,state:pixelLive.effectiveState||pixelLive.state}:{state:selected,percent:selected==='preparing'?0:42,model:'P1S',clock:'12:34',leftMinutes:130,nozzle:215,bed:60,temperature:24.6,humidity:46,stage:-1,currentLayer:selected==='preparing'?0:84,totalLayers:200,filamentColor:0xff9000,feedColor:0xff9000,filamentKnown:true};
}
function pixelCancelPreview(){if(pixelAnimation)clearInterval(pixelAnimation);pixelAnimation=0;}
function pixelPaint(){
 if(!pixelVisible()){pixelCancelPreview();return;}
 const source=pixelPreviewSnapshot(),layout=$('pixelLayout').value,idle=$('pixelIdle').checked;
 const key=JSON.stringify([source.state,layout,$('pixelState').value,idle]);
 if(key!==pixelAnimationKey){pixelTick=0;pixelAnimationKey=key;}
 let value=source;
 if($('pixelState').value!=='live'&&source.state==='idle'&&source.temperature!=null&&source.humidity!=null&&idle&&Math.floor(pixelTick/100)%2)value={...source,state:'environment'};
 const tick=$('pixelState').value==='live'?Math.floor(((source.stateAgeMs||0)+performance.now()-(pixelLive.receivedAt||performance.now()))/100):pixelTick;
 PixelArt.paint($('pixelCanvas'),value,layout,tick);
 $('pixelFeedback').textContent=t('预览自动刷新，内置时钟字形以设备为准。','Preview updates automatically; the built-in clock font depends on the device.');
}
function pixelSchedulePreview(){
 if(!pixelVisible()){pixelCancelPreview();return;}
 pixelPaint();
 if(!pixelAnimation&&!window.matchMedia('(prefers-reduced-motion: reduce)').matches)
  pixelAnimation=setInterval(()=>{++pixelTick;pixelPaint();},100);
}
function updatePixelScreenPanel(value){
 if(!value)return;pixelLive={...value,receivedAt:performance.now()};
 if(!pixelCard){
  pixelCard=document.createElement('section');pixelCard.id='pixelScreen';
  pixelCard.innerHTML='<div class="sectionHead"><h2 id="pixelTitle"></h2><span id="pixelStatus" class="badge" role="status"></span></div><p id="pixelHint" class="muted"></p><div class="settingsGroups pixelPanel"><fieldset class="settingsGroup"><legend id="pixelSettingsTitle"></legend><label class="check"><input data-pixel id="pixelEnabled" type="checkbox"><span id="pixelEnabledText"></span></label><details id="pixelConnection" open><summary id="pixelConnectionText"></summary><label><span id="pixelDeviceText"></span><select data-pixel id="pixelDevice"></select></label><button type="button" id="pixelScan"></button><label id="pixelManualRow" hidden><span id="pixelMacText"></span><input data-pixel id="pixelMac" maxlength="17" autocomplete="off" spellcheck="false" placeholder="AA:BB:CC:DD:EE:FF"></label><div class="passwordField"><span id="pixelPasswordText"></span><div class="passwordActions"><label id="pixelClearRow" class="checkbox"><input data-pixel id="pixelClearPassword" type="checkbox"><span id="pixelClearText"></span></label><label class="checkbox" id="pixelEncryptLabel"><input data-pixel id="pixelEncrypt" type="checkbox"><span id="pixelEncryptText"></span></label></div><input data-pixel id="pixelPassword" aria-labelledby="pixelPasswordText" type="password" inputmode="numeric" maxlength="6" pattern="[0-9]{6}" autocomplete="new-password"></div><p id="pixelPasswordHint" class="muted"></p></details><div class="pixelControlRow"><label class="check"><input data-pixel id="pixelPower" type="checkbox"><span id="pixelPowerText"></span></label><label class="check"><input data-pixel id="pixelRotate" type="checkbox"><span id="pixelRotateText"></span></label><label class="check" hidden><input data-pixel id="pixelReset" type="checkbox"><span id="pixelResetText"></span></label><label class="check" hidden><input data-pixel id="pixelClearMaterials" type="checkbox"><span id="pixelClearMaterialsText"></span></label><label class="check"><input data-pixel id="pixelIdle" type="checkbox"><span id="pixelIdleText"></span></label></div><label><span class="settingValue"><span id="pixelBrightnessText"></span><output id="pixelBrightnessValue"></output></span><input data-pixel class="settingRange" id="pixelBrightness" type="range" min="5" max="100" step="1" value="80"></label><p id="pixelControlHint" class="muted"></p><label><span id="pixelLayoutText"></span><select data-pixel id="pixelLayout"><option value="printer"></option><option value="progress"></option></select></label><p id="pixelLayoutHelp" class="muted"></p><button id="pixelSave" class="primary"></button></fieldset><fieldset class="settingsGroup pixelPreviewPane"><legend id="pixelPreviewTitle"></legend><div class="pixelPreviewContent"><label><span id="pixelStateText"></span><select data-pixel id="pixelState"></select></label><div class="pixelPreviewStage"><div class="pixelProduct"><canvas id="pixelCanvas" width="320" height="320" class="pixelPreviewCanvas" role="img" aria-label="32 × 32 LED"></canvas></div></div><p id="pixelClockHint" class="muted"></p><p id="pixelCaption" class="muted"></p><p id="pixelFeedback" class="muted" role="status"></p></fieldset></div>';
  $('settings').append(pixelCard);fieldHeadingObserver.observe($('pixelPassword').parentElement);setupOptionalPanel(pixelCard,open=>{if(open){pixelSchedulePreview();if(pixelScanBusy)pixelScanPoll();}else{pixelCancelPreview();pixelScanPause();}});
  for(const id of ['pixelMac','pixelPassword','pixelClearPassword','pixelEnabled','pixelLayout','pixelBrightness','pixelPower','pixelRotate','pixelIdle','pixelReset','pixelClearMaterials'])for(const event of ['input','change','locked'])$(id).addEventListener(event,pixelButtons);
  $('pixelDevice').onchange=()=>{const selected=$('pixelDevice').value;$('pixelManualRow').hidden=selected!=='__manual__';if(selected==='__manual__'){$('pixelMac').focus();return;}if(!selected)return;$('pixelMac').value=selected;pixelButtons();};
  $('pixelEncrypt').addEventListener('change',pixelButtons);
  $('pixelClearPassword').addEventListener('change',()=>{if($('pixelClearPassword').checked)$('pixelPassword').value='';pixelButtons();});
  $('pixelMac').addEventListener('input',pixelDeviceOptions);
  $('pixelScan').onclick=async()=>{
   if(pixelBusy||pixelScanBusy)return;pixelScanError="";pixelScanBusy=true;pixelButtons();
   try{await api('/api/pixel-screen/scan',{});await pixelScanPoll();}
   catch(error){pixelScanBusy=false;pixelScanError=error.code==='radio_busy'?t('蓝牙正在连接或扫描，请稍后重试。','Bluetooth is connecting or scanning. Please retry shortly.'):failure(error);pixelButtons();}
  };
  for(const id of ['pixelLayout','pixelIdle'])$(id).addEventListener('change',pixelSchedulePreview);$('pixelState').onchange=pixelSchedulePreview;
  for(const tab of document.querySelectorAll('[data-tab]'))tab.addEventListener('click',()=>{if(pixelVisible()){pixelSchedulePreview();if(pixelScanBusy)pixelScanPoll();}else{pixelCancelPreview();pixelScanPause();}});
  document.addEventListener('visibilitychange',()=>{if(pixelVisible()){pixelSchedulePreview();if(pixelScanBusy)pixelScanPoll();}else{pixelCancelPreview();pixelScanPause();}});
  $('pixelSave').onclick=async()=>{
   if(pixelBusy)return;pixelBusy=true;pixelButtons();
   try{if(pixelDraft().mac!==pixelKnown?.mac||pixelDraft().password)pixelConfirmedMac='';await api('/api/pixel-screen',pixelDraft());pixelFields(await api('/api/pixel-screen'));pixelSchedulePreview();$('pixelFeedback').textContent=t('已保存；按绑定的MAC连接，受保护设备先验证密码。','Saved; connects to the bound MAC and authenticates protected devices first.');}
   catch(error){$('pixelFeedback').textContent=failure(error);}
   finally{pixelBusy=false;pixelButtons();}
  };
  pixelFields(value);
 }
 if(pixelKnown&&pixelKnown.mac===value.mac){
  if($('pixelPower').checked===(pixelKnown.screenOn??true))$('pixelPower').checked=value.screenOn??true;
  pixelKnown.screenOn=value.screenOn??true;pixelKnown.displayControlAvailable=!!value.displayControlAvailable;
 }
 if(value.connected&&value.authenticated&&value.controlsApplied&&!pixelBusy&&!pixelChanged()&&pixelConfirmedMac!==value.mac){$('pixelConnection').open=false;pixelConfirmedMac=value.mac;}
 const prompt=t('扫描后选择像素屏','Scan and select a pixel screen');if($('pixelDevice').dataset.prompt!==prompt){pixelDeviceOptions();$('pixelDevice').dataset.prompt=prompt;}
 $('pixelPassword').placeholder=pixelKnown?.passwordSaved?t('已保存；同一MAC留空保留','Saved; blank keeps it for the same MAC'):t('未设置密码可留空','Leave blank if no device password');
 $('pixelTitle').textContent=t('像素屏（32×32）','Pixel screen (32×32)');
 const labels={disabled:['未启用','Disabled'],waiting_radio:['蓝牙启动中','Starting Bluetooth'],searching:['搜索像素屏','Searching'],connecting:['连接中','Connecting'],discovering:['识别服务','Discovering services'],subscribing:['订阅通知','Subscribing'],identifying:['读取设备信息','Identifying device'],authenticating:['验证密码','Authenticating'],resetting:['重置像素屏','Resetting display'],reset_failed:['重置失败','Reset failed'],password_restore_failed:['密码恢复失败','Password restore failed'],upload_rejected:['像素屏拒绝画面','Display rejected image'],no_space:['像素屏存储不足','Display storage full'],clearing:['清理已存素材','Clearing stored artwork'],clear_failed:['素材清理失败','Artwork cleanup failed'],ready:['已连接','Connected'],uploading:['画面更新中','Updating display'],retry_wait:['等待重连','Waiting to reconnect'],password_required:['需要六位设备密码','Device password required'],password_rejected:['设备密码错误','Incorrect device password'],password_unavailable:['密码不可用，请重新填写','Re-enter device password'],authentication_unknown:['认证未确认','Authentication unconfirmed'],unsupported_screen:['不支持的像素屏规格','Unsupported display size'],memory_wait:['等待可用内存','Waiting for memory'],error:['连接模块异常','Connection module error']};const label=value.enabled&&value.sourceSettling?['等待设备稳定','Waiting for stable printer']:labels[value.connectionState]||['未连接','Disconnected'];$('pixelStatus').textContent=t(...label);$('pixelStatus').classList.toggle('ok',value.connectionState==='ready'&&value.authenticated);
 $('pixelHint').textContent=t('启用后连接绑定iDotMatrix像素屏设备并同步状态；停用会断开连接，保留设置。','Enable to connect and sync the bound iDotMatrix pixel display. Disable to disconnect and keep its settings.');
 $('pixelSettingsTitle').textContent=t('联动设置','Link settings');$('pixelPreviewTitle').textContent=t('画面预览','Display preview');
 $('pixelEnabledText').title=t('停用联动不会熄屏；熄屏请关闭显示开关。','Disabling linking does not turn off the display. Use the display switch to turn it off.');
 $('pixelEnabledText').textContent=t('启用像素屏联动','Enable pixel screen linking');
 $('pixelDeviceText').textContent=t('选择像素屏','Select pixel screen');$('pixelScan').textContent=pixelScanBusy?t('正在扫描… ','Scanning… ').trim():pixelScanError?t('扫描失败，请重试','Scan failed; retry'):t('扫描附近像素屏','Scan nearby pixel screens');$('pixelScan').title=pixelScanError;$('pixelConnectionText').textContent=t('连接设置','Connection settings');
 $('pixelMacText').textContent=t('像素屏 MAC 地址','Pixel screen MAC address');
 const protectionHint=t('勾选后加密保存，并绑定当前设备密钥；重置或更换设备后需重新填写。未勾选则明文保存。','Encrypts this credential and binds it to this device key. Re-enter it after a reset or on another device. Unchecked: stored as plaintext.');$('pixelEncryptLabel').title=protectionHint;$('pixelEncrypt').title=protectionHint;$('pixelEncryptText').textContent=t('加密并绑定','Encrypt & bind');
 $('pixelPasswordText').textContent=t('设备密码','Device password');$('pixelClearText').textContent=t('清除密码','Clear password');$('pixelPasswordHint').textContent=t('6位数字，留空保留；不修改像素屏密码。','Six digits; blank keeps the saved password. Does not change the display password.');
 $('pixelPowerText').textContent=t('显示开关','Display switch');$('pixelRotateText').textContent=t('旋转180°','Rotate 180°');$('pixelBrightnessText').textContent=t('像素屏亮度','Pixel screen brightness');$('pixelControlHint').textContent=t('保存后生效，重新连接后自动恢复。','Applied after saving and restored after reconnecting.');
 $('pixelResetText').textContent=t('连接后自动重置','Reset on connection');$('pixelClearMaterialsText').textContent=t('连接后自动清理','Clear artwork on connection');
 $('pixelIdleText').textContent=t('待机显示温湿度','Temperature & humidity while idle');
 $('pixelLayoutText').textContent=t('打印中显示方式','Printing display style');$('pixelLayout').options[0].textContent=t('彩色打印机','Colour printer');$('pixelLayout').options[1].textContent=t('进度视图','Progress view');
 $('pixelClockHint').textContent=t('内置时钟自动走时，无需每分钟传图；字形以设备为准。','The built-in clock runs locally without minute-by-minute uploads; its font depends on the device.');
 $('pixelCaption').textContent=t('32×32 像素 · 动画循环播放','32×32 pixels · Looping animation');
 $('pixelStateText').textContent=t('预览内容','Preview content');$('pixelSave').textContent=t('保存设置','Save settings');
 const states=[['live',t('当前打印机（跟随状态）','Current printer (live)')],['idle',t('示例：空闲','Example: Idle')],['preparing',t('示例：准备中','Example: Preparing')],['printing',t('示例：打印中 · 42%','Example: Printing · 42%')],['paused',t('示例：已暂停','Example: Paused')],['complete',t('示例：已完成','Example: Complete')],['cancelled',t('示例：已取消','Example: Cancelled')],['error',t('示例：故障','Example: Error')],['offline',t('示例：离线','Example: Offline')],['environment',t('示例：温湿度','Example: Temperature & humidity')]];
 const select=$('pixelState'),selected=select.value;
 if(select.options.length!==states.length)select.replaceChildren(...states.map(([value,label])=>new Option(label,value)));
 else states.forEach(([,label],i)=>{if(select.options[i].textContent!==label)select.options[i].textContent=label;});
 select.value=selected||'printing';
 pixelButtons();pixelSchedulePreview();
}
window.XTouchPixelPortal.update=updatePixelScreenPanel;
if(window.pixelScreenStatus)updatePixelScreenPanel(window.pixelScreenStatus);
