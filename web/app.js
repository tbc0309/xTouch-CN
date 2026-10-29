import boardTargets from './board-targets.js';
import testBoardTargets from './test-board-targets.js';
import {installerChannel,validateFirmwareCatalog} from './installer-channel.js';
import {websiteLanguage,saveWebsiteLanguage} from './language.js';
import {renderFooter} from './page-footer.js';
const words = {
  testChannel: ['测试版', 'Test firmware'],
  testWarning: ['正在选择测试固件。请核对完整主板型号、屏幕与芯片修订，安装前备份配置。实测状态请查看测试版主板列表。', 'Test firmware selected. Check the exact board, display and chip revision, and back up your settings. See the test board list for validation status.'],
  testUnavailable: ['此测试固件目录暂不可用，未切换到正式版。请稍后重试。', 'This test firmware catalog is unavailable. No release firmware was substituted. Please retry later.'],
  testRebuilding: ['此版本测试固件正在重新编译，暂不可安装。', 'Test firmware for this version is being rebuilt and is not yet available for installation.'],
  "errorLookup": [
    "故障码查询",
    "Error lookup"
  ],
  "projectLink": [
    "项目地址",
    "Project"
  ],
  "eyebrow": [
    "在线安装 · 扫码配网",
    "INSTALL · CONNECT · GET STARTED"
  ],
  "title": [
    "在线安装 xTouch-CN",
    "Install xTouch-CN online"
  ],
  "intro": [
    "选择对应主板固件，刷写后扫码配网，在网页管理打印机。",
    "Select your board, install, scan to join Wi-Fi, and manage printers in your browser."
  ],
  "usbTag": [
    "USB 在线安装",
    "USB WEB INSTALL"
  ],
  "usbTitle": [
    "连接并刷写",
    "Connect and install"
  ],
  "boardSelect": [
    "选择主板",
    "Select board"
  ],
  "usb1": [
    "电脑使用 Chrome 或 Edge，关闭占用串口的软件。",
    "Use desktop Chrome or Edge. Close software using the serial port."
  ],
  "usb2": [
    "确认主板型号，加载刷写工具，再选择它的串口。",
    "Check the board model, load the installer, then select its serial port."
  ],
  "usb3": [
    "按提示安装，CYD 首次开机需触摸校准，再扫码配网。",
    "Follow the installation dialog. CYD needs first-boot touch calibration. Then scan to join Wi-Fi."
  ],
  "erase": [
    "保留设置时不要勾选 Erase device（擦除设备）；勾选会清空内部配置和校准。SD 备份可能在开机时恢复，全新配置请先移除 SD 卡。刷写期间不要断电、拔线或关闭页面。",
    "To keep settings, leave “Erase device” unchecked. Erasing removes internal settings and calibration. SD backups may be restored at startup; remove SD for a fresh setup. Do not unplug USB, turn off power or close the page while installing."
  ],
  "loadInstaller": [
    "加载刷写工具",
    "Load installer"
  ],
  "installerEnglish": [
    "刷写工具的连接、进度和确认窗口由 ESP Web Tools 提供，目前为英文。",
    "Connection, progress and confirmation dialogs are supplied by ESP Web Tools in English."
  ],
  "setupTitle": [
    "扫码配网与初始化",
    "Wi-Fi and setup"
  ],
  "setup1": [
    "CYD 首次开机需触摸校准；S3 V2/P4/ES3C35P 无需校准。",
    "Calibrate CYD on first boot; S3 V2/P4/ES3C35P need no calibration."
  ],
  "setup2": [
    "扫描屏幕二维码连接 XTOUCH-CN 热点；密码 12345678。",
    "Scan the screen QR code to join the XTOUCH-CN hotspot. Password: 12345678."
  ],
  "setup3": [
    "打开 192.168.4.1，连接家庭 2.4GHz Wi-Fi。",
    "Open 192.168.4.1 and join your 2.4GHz Wi-Fi."
  ],
  "setup4": [
    "在设置打开网页管理扫码进入后台；初始密码 12345678，首次登录修改后添加局域网或云设备。",
    "Open Web management in Settings and scan its QR code. Sign in with 12345678, change it, then add LAN or cloud printers."
  ],
  "otaHint": [
    "已安装 CN：可在设置开启 OTA，或连续点击版本号三次检查更新。",
    "Already on CN? Enable OTA in Settings or tap the version three times to check for updates."
  ],
  "boardHint": [
    "仅支持 ESP32-2432S028R：ILI9341 显示 + XPT2046 电阻触摸，4MB Flash。",
    "For ESP32-2432S028R: ILI9341 display, XPT2046 resistive touch and 4MB Flash."
  ],
  "version": [
    "固件",
    "Firmware"
  ],
  "connect": [
    "选择串口并连接",
    "Select port and connect"
  ],
  "unavailable": [
    "刷写资源暂不可用，请刷新重试或前往 Releases。",
    "Installer assets are unavailable. Reload or visit Releases."
  ],
  "unsupported": [
    "请在电脑 Chrome / Edge 中通过 HTTPS 打开此页面。",
    "Open this page over HTTPS in desktop Chrome or Edge."
  ],
  "loading": [
    "正在加载刷写工具…",
    "Loading installer…"
  ],
  "recoveryHint": [
    "连接失败时，先关闭其他占用串口的软件或网页。S3/P4 确认已开机，按住 BOOT、点按 RESET、松开 BOOT，再重新连接。写入中断后可重新完整安装。",
    "If connection fails, close other serial tools or tabs. Power on S3/P4, hold BOOT, tap RESET, release BOOT and reconnect. If writing is interrupted, repeat the complete installation."
  ],
  "loadFailed": [
    "工具加载失败，请检查网络后重试，或使用 SD 卡升级。",
    "Installer could not load. Check the network and retry, or use SD installation."
  ]
};
let lang=websiteLanguage(), metadata=null, installerReady=false, boardCatalog=[];
let channel=installerChannel(false), activeTargets=boardTargets, firmwareRequest=0, requestController=null;
const boardPreferenceKey=()=>"xTouch-CN.installer.board"+(channel.id==='test'?'.test':'');
function rememberedBoard(){try{return localStorage.getItem(boardPreferenceKey());}catch{return null;}}
const $=id=>document.getElementById(id), t=key=>(words[key]||[key,key])[lang==="en"?1:0];
function text(tag,value,className){const el=document.createElement(tag);el.textContent=value;if(className)el.className=className;return el;}
function boardName(board){return board.name.replace(/^Waveshare · /,lang==="zh"?"微雪 · ":"Waveshare · ");}
function applyLanguage(){
 renderFooter(lang);
 $("language").value=lang;document.documentElement.lang=lang==="zh"?"zh-CN":"en";
 document.title=lang==="zh"?"xTouch-CN · 在线刷写":"xTouch-CN · Web installer";
 document.querySelectorAll("[data-i18n]").forEach(el=>el.textContent=t(el.dataset.i18n));
 if(metadata)$("version").textContent=`${t("version")} v${metadata.version}${channel.id==="test"?" · "+t("testChannel"):""}`;
 if(installerReady && $("connect-installer"))$("connect-installer").textContent=t("connect");
 if(metadata){
  $("board-name").textContent=boardName(metadata);
  for(const option of $("board").options){const board=boardCatalog.find(b=>b.id===option.value);if(board)option.textContent=boardName(board);}
  $("board-screen").textContent=activeTargets[metadata.id].screen.join(" × ");
  const hint=activeTargets[metadata.id].hint;
  document.querySelector('[data-i18n="boardHint"]').textContent=hint?hint[lang==="zh"?0:1]:t("boardHint");
 }
}
function selectBoard(){
 metadata=boardCatalog.find(board=>board.id===$("board").value);
 if(metadata)try{localStorage.setItem(boardPreferenceKey(),metadata.id);}catch{}
 installerReady=false;$("installer-widget").replaceChildren();$("load-installer").disabled=!metadata;
 $("load-installer").hidden=false;
 $("installer-status").textContent="";applyLanguage();
}
$("board").addEventListener("change",selectBoard);
$("language").addEventListener("change",()=>{lang=$("language").value;saveWebsiteLanguage(lang);applyLanguage();});
$("test-channel").addEventListener("change",()=>{
 channel=installerChannel($("test-channel").checked);
 activeTargets=channel.id==='test'?testBoardTargets:boardTargets;
 $("usb-install-card").classList.toggle('test-channel',channel.id==='test');
 $("test-channel-note").hidden=channel.id!=='test';
 initFirmware();
});
async function initFirmware() {
  const request=++firmwareRequest;
  requestController?.abort();
  const controller=requestController=new AbortController();
  const selectedChannel=channel, targets=activeTargets;
  metadata=null;boardCatalog=[];installerReady=false;
  $("board").replaceChildren();$("board").disabled=true;
  $("installer-widget").replaceChildren();$("load-installer").disabled=true;
  $("load-installer").hidden=false;$("version").textContent='—';
  $("board-name").textContent=selectedChannel.id==='test'?t('testChannel'):'xTouch-CN';
  $("board-screen").textContent='—';$("installer-status").textContent=t('loading');
  const timer=setTimeout(()=>controller.abort(),10000);
  try {
    const response=await fetch(selectedChannel.index,{cache:'no-cache',signal:controller.signal});
    if(!response.ok)throw Error('Firmware catalog unavailable');
    const data=await response.json();
    if(request!==firmwareRequest)return;
    if(selectedChannel.id==='test' && /^\d+\.\d+\.\d+-test$/.test(data.version) && data.releaseTag===data.version && data.prerelease===true && Array.isArray(data.boards) && !data.boards.length){
      $("version").textContent=`${t('version')} v${data.version} · ${t('testChannel')}`;
      $("installer-status").textContent=t('testRebuilding');return;
    }
    const catalog=validateFirmwareCatalog(data,targets,selectedChannel);
    if(request!==firmwareRequest)return;
    boardCatalog=catalog;
    $("board").replaceChildren(...catalog.map(board=>{const option=text('option',boardName(board));option.value=board.id;return option;}));
    const savedBoard=rememberedBoard();
    if(catalog.some(board=>board.id===savedBoard))$("board").value=savedBoard;
    $("board").disabled=false;selectBoard();
  } catch {
    if(request!==firmwareRequest)return;
    $("installer-status").textContent=t(selectedChannel.id==='test'?'testUnavailable':'unavailable');
  } finally {clearTimeout(timer);}
}
$("load-installer").addEventListener("click", async () => {
  if (!window.isSecureContext || !("serial" in navigator)) {
    $("installer-status").textContent = t("unsupported");
    return;
  }
  if (!metadata) return;
  $("load-installer").disabled = true;
  $("board").disabled = true;
  $("test-channel").disabled = true;
  $("installer-status").textContent = t("loading");
  let timer;
  try {
    await Promise.race([
      import("./vendor/install-button.js"),
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(Error("installer timeout")), 20000);
      }),
    ]);
    await customElements.whenDefined("esp-web-install-button");
    const installer = document.createElement("esp-web-install-button");
    installer.setAttribute("manifest", metadata.manifest);
    const connect = text("button", t("connect"), "button primary");
    connect.id = "connect-installer";
    connect.slot = "activate";
    installer.append(connect);
    $("installer-widget").replaceChildren(installer);
    $("installer-status").textContent = "";
    installerReady = true;
    $("load-installer").hidden = true;
  } catch {
    $("load-installer").disabled = false;
    $("installer-status").textContent = t("loadFailed");
  } finally {
    clearTimeout(timer);
    $("board").disabled = false;
    $("test-channel").disabled = false;
  }
});
new MutationObserver(()=>{
 const active=!!document.querySelector('ewt-install-dialog');
 $("test-channel").disabled=active;
 $("board").disabled=active||!metadata;
}).observe(document.body,{childList:true});
applyLanguage();
initFirmware();
